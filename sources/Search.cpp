#include "../headers/Search.h"
#include "../headers/Bitboard.h"
#include "../headers/Clock.h"
#include "../headers/Evaluation.h"
#include "../headers/Evaluator.h"
#include "../headers/Move.h"
#include "../headers/See.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <thread>
#include <vector>

namespace maharajah {

namespace {

// most valuable victim / least valuable attacker, indexed [attacker][victim]
constexpr auto mvv_lva = [] {
  std::array<std::array<int, PieceCount::all>, PieceCount::all> table{ };
  const auto value = [](const int piece) {
    return Evaluation::piece_value[piece] / 100 + 1;
  };

  for(int attacker{ }; attacker < PieceCount::all; ++attacker) {
    for(int victim{ }; victim < PieceCount::all; ++victim)
      table[attacker][victim] = value(victim) * 100 + (10 - value(attacker));
  }
  return table;
}();

// late-move reductions in plies, indexed [depth][moves searched]
const auto late_move_reductions = [] {
  std::array<std::array<int, Limits::max_moves>, Limits::max_ply + 1> table{ };
  for(int depth{ 1 }; depth <= Limits::max_ply; ++depth) {
    for(int moves{ 1 }; moves < Limits::max_moves; ++moves)
      table[depth][moves] = static_cast<int>(0.75 + std::log(depth) * std::log(moves) / 2.25);
  }
  return table;
}();

} // namespace

void TimeControl::set_movetime(const int movetime_ms) {
  reset();
  if(movetime_ms <= 0)
    return;

  starttime = now_ms();
  stoptime = starttime + movetime_ms;
  timeset = true;
}

void Search::reset() {
  nodes_ = 0;
  reported_nodes_ = 0;
  if(!helper_)
    engine_.time_control.stopped = false;
  follow_pv_ = false;
  score_pv_ = false;
  ply_ = 0;
  root_count_ = 0;

  killer_moves_ = { };
  history_moves_ = { };
  pv_table_ = { };
  pv_length_ = { };
  root_moves_ = { };
  root_scores_ = { };
}

void Search::communicate() {
  if(helper_) {
    report_nodes();
    return;
  }

  TimeControl& time = engine_.time_control;

  if(time.timeset && now_ms() > time.stoptime)
    time.stopped = true;

  if(time.poll_input && time.poll)
    time.poll(time);
}

void Search::report_nodes() {
  engine_.time_control.helper_nodes.fetch_add(nodes_ - reported_nodes_, std::memory_order_relaxed);
  reported_nodes_ = nodes_;
}

int Search::evaluate() const {
  // the NNUE backend is not available yet, so every mode evaluates classically
  return Evaluator::evaluate(board_.state);
}

int Search::effective_depth(const int depth) const {
  const int cap = engine_.search_config.max_depth_cap;

  if(depth <= 0)
    return 1;
  if(cap == Limits::max_ply || depth < cap)
    return depth;
  return cap;
}

bool Search::is_insufficient_material() const {
  const BoardState& state = board_.state;
  if(!state.standard_rules)
    return false;

  const auto count = [&](const Pieces piece) { return count_bits(state.bitboards[piece]); };

  for(const Pieces piece : { P, p, R, r, Q, q, A, a, C, c, M, m }) {
    if(count(piece))
      return false;
  }

  const int white_knights = count(N);
  const int black_knights = count(n);
  const int white_bishops = count(B);
  const int black_bishops = count(b);
  const int white_minors = white_knights + white_bishops;
  const int black_minors = black_knights + black_bishops;

  // Dead positions only: K vs K, K + single minor vs K, and KB vs KB with both
  // bishops on the same square colour. KNN vs K, KN vs KN and KB vs KN are not dead.
  if(white_minors == 0 && black_minors == 0)
    return true;

  if(white_minors + black_minors == 1)
    return true;

  if(white_knights == 0 && black_knights == 0 && white_bishops == 1 && black_bishops == 1) {
    const int white_square = get_ls1b_index(state.bitboards[B]);
    const int black_square = get_ls1b_index(state.bitboards[b]);
    // square colour is the parity of rank + file
    return (((white_square >> 3) ^ white_square) & 1) == (((black_square >> 3) ^ black_square) & 1);
  }

  return false;
}

bool Search::is_draw() const {
  const BoardState& state = board_.state;
  const int repetitions = board_.repetition_count();

  // threefold and fivefold repetition, fifty and seventy-five move rules
  return repetitions >= 3 || (state.standard_rules && state.halfmove >= 100) || is_insufficient_material();
}

bool Search::should_return_draw_score() const {
  return ply_ != 0 && is_draw();
}

void Search::print_info(std::ostream& out, const int score, const int depth, const int elapsed) const {
  const u64 nodes = nodes_ + engine_.time_control.helper_nodes.load(std::memory_order_relaxed);
  if(score > -mate_value && score < -mate_score)
    out << std::format("info score mate {} depth {} nodes {} time {} pv ", -(score + mate_value) / 2 - 1, depth, nodes, elapsed);
  else if(score > mate_score && score < mate_value)
    out << std::format("info score mate {} depth {} nodes {} time {} pv ", (mate_value - score) / 2 + 1, depth, nodes, elapsed);
  else
    out << std::format("info score cp {} depth {} nodes {} time {} pv ", score, depth, nodes, elapsed);

  for(int count{ }; count < pv_length_[0]; ++count)
    out << Board::move_to_string(pv_table_[0][count]) << ' ';

  out << '\n' << std::flush;
}

SearchResult Search::run(const int depth, std::ostream* info) {
  const int start = now_ms();
  SearchResult result{ };
  reset();

  int alpha = -infinity;
  int beta = infinity;
  int window = aspiration_window;
  const int max_depth = effective_depth(depth);

  // iterative deepening; odd helpers start a ply deeper so the threads spread over
  // two depths instead of all searching the same tree
  for(int current_depth{ 1 + thread_index_ % 2 }; current_depth <= max_depth; ++current_depth) {
    if(stopped())
      break;

    follow_pv_ = true;
    const int score = negamax(alpha, beta, current_depth);

    if(stopped())
      break;

    // Aspiration window failed: re-search the same depth with the failed bound moved
    // past the fail-soft score by a step that doubles on every retry.
    if(score <= alpha && alpha > -infinity) {
      alpha = std::max(score - window, -infinity);
      window *= 2;
      --current_depth;
      continue;
    }
    if(score >= beta && beta < infinity) {
      beta = std::min(score + window, +infinity);
      window *= 2;
      --current_depth;
      continue;
    }

    window = aspiration_window;
    alpha = score - window;
    beta = score + window;
    result.score = score;
    result.depth = current_depth;

    if(info && pv_length_[0])
      print_info(*info, score, current_depth, now_ms() - start);

    if(!helper_ && root_count_ > 0)
      result.best_move = select_skill_move();
  }

  if(helper_)
    report_nodes();

  result.nodes = nodes_;
  return result;
}

namespace {

// Helper search threads, stopped and joined when this goes out of scope.
class HelperThreads {
  public:
  HelperThreads(Engine& engine, const int count)
      : engine_(engine)
      , boards_(static_cast<std::size_t>(count), engine.board) {
    threads_.reserve(boards_.size());
    try {
      for(int index{ }; index < count; ++index) {
        threads_.emplace_back([&engine, &board = boards_[static_cast<std::size_t>(index)], index] {
          Search(engine, board, index + 1).run(Limits::max_ply);
        });
      }
    } catch(...) {
      // the destructor does not run for a half-built object
      stop_and_join();
      throw;
    }
  }

  HelperThreads(const HelperThreads&) = delete;
  HelperThreads& operator=(const HelperThreads&) = delete;

  ~HelperThreads() {
    stop_and_join();
  }

  private:
  void stop_and_join() {
    engine_.time_control.stopped = true;
    for(std::thread& thread : threads_)
      thread.join();
  }

  Engine& engine_;
  std::vector<Board> boards_;
  std::vector<std::thread> threads_;
};

} // namespace

SearchResult run_search(Engine& engine, const int depth, std::ostream* info) {
  TimeControl& time = engine.time_control;
  time.helper_nodes = 0;

  const int helper_count = std::clamp(engine.threads, Engine::min_threads, Engine::max_threads) - 1;
  if(helper_count == 0)
    return Search(engine).run(depth, info);

  // cleared before the helpers start: the main search clears it only once it runs
  time.stopped = false;

  SearchResult result;
  {
    const HelperThreads helpers(engine, helper_count);
    result = Search(engine).run(depth, info);
  }

  result.nodes += time.helper_nodes;
  return result;
}

// negamax alpha beta search
int Search::negamax(int alpha, int beta, int depth) {
  pv_length_[ply_] = ply_;
  if(ply_ == 0)
    root_count_ = 0;

  int score;
  HashFlag hash_flag = HashFlag::alpha;
  const Colors side = board_.state.side;
  const u64 own_king = board_.state.bitboards[side == white ? K : k];

  if(should_return_draw_score())
    return 0;

  const bool pv_node = beta - alpha > 1;
  const TranspositionTable::Probe hash_entry = engine_.transposition_table.probe(board_.state.hash_key, alpha, beta, depth, ply_);
  if(ply_ && !pv_node && hash_entry.score != TranspositionTable::no_entry)
    return hash_entry.score;

  if((nodes_ & 2047) == 0)
    communicate();

  if(depth == 0)
    return quiescence(alpha, beta);

  if(ply_ > Limits::max_ply - 1)
    return evaluate();

  // the king has been captured
  if(!own_king)
    return -mate_value + ply_;

  ++nodes_;

  const bool in_check = board_.is_square_attacked(get_ls1b_index(own_king), opponent(side));
  int static_eval{ };
  bool has_static_eval{ };

  // check extension
  if(in_check) {
    ++depth;
  } else {
    static_eval = evaluate();
    has_static_eval = true;
  }

  const SearchConfig& config = engine_.search_config;

  // internal iterative reduction: without a stored move the move ordering is poor,
  // so search shallower; the next iteration finds the move in the table
  if(depth >= 4 && ply_ && !hash_entry.move)
    --depth;

  // reverse futility pruning
  if(depth <= 2 && ply_ && !in_check && !pv_node) {
    if(static_eval - config.reverse_futility_margin_per_depth * depth >= beta)
      return static_eval;
  }

  int legal_moves{ };

  // Null-move pruning is unsound in zugzwang-prone positions; require the side to
  // move to have at least one piece besides king and pawns.
  const auto& bb = board_.state.bitboards;
  const u64 own_non_pawn_material =
      (side == white) ? (bb[N] | bb[B] | bb[R] | bb[Q] | bb[A] | bb[C] | bb[M]) : (bb[n] | bb[b] | bb[r] | bb[q] | bb[a] | bb[c] | bb[m]);

  // the reduction grows with depth; only tried where the static evaluation already
  // beats beta, outside PV nodes
  if(depth >= 3 && !in_check && ply_ && !pv_node && own_non_pawn_material && static_eval >= beta) {
    const ZobristKeys& keys = ZobristKeys::get();
    board_.push_state();
    ++ply_;
    board_.remember_position();

    BoardState& state = board_.state;
    if(state.en_passant != no_square)
      state.hash_key ^= keys.en_passant[state.en_passant];
    state.en_passant = no_square;
    state.side = opponent(state.side);
    state.hash_key ^= keys.side;

    score = -negamax(-beta, -beta + 1, std::max(0, depth - 1 - (2 + depth / 6)));

    --ply_;
    board_.forget_position();
    board_.pop_state();

    if(stopped())
      return 0;
    // an unverified mate from the null move is not returned
    if(score >= beta)
      return score >= mate_score ? beta : score;
  }

  MoveList moves_list;
  board_.generate_moves(moves_list);

  if(follow_pv_)
    enable_pv_scoring(moves_list);

  sort_moves(moves_list, hash_entry.move);

  int best_move{ };
  int best_score{ -infinity };
  int moves_searched{ };
  // quiet moves searched without a cutoff, for the history malus
  std::array<int, Limits::max_moves> quiet_moves;
  int quiet_count{ };
  for(size_t i{ }; i < moves_list.size(); ++i) {
    const int move = moves_list[i];
    const bool capture = Move::get_move_capture(move);
    const bool is_quiet_move = !capture && !Move::is_promotion(move);
    const bool prunable = depth <= 2 && ply_ && !in_check && !pv_node && is_quiet_move;

    // Futility-prune only after a legal move has been searched: pruning every move
    // would fall through to the no-legal-moves branch and misreport mate/stalemate.
    if(prunable && has_static_eval && legal_moves > 0 && static_eval + config.futility_margin_per_depth * depth <= alpha)
      continue;

    // late move pruning
    if(prunable && moves_searched >= config.late_move_pruning_base + config.late_move_pruning_scale * depth)
      continue;

    ++ply_;
    board_.remember_position();

    if(!board_.make_move(move, TypeMove::all_moves)) {
      --ply_;
      board_.forget_position();
      continue;
    }

    ++legal_moves;

    if(moves_searched == 0) {
      // full window search for the first move
      score = -negamax(-beta, -alpha, depth - 1);
    } else {
      // late move reduction: at least one ply, less in PV nodes, never into quiescence
      if(moves_searched >= full_depth_moves && depth >= reduction_limit && !in_check && is_quiet_move) {
        const int reduction = late_move_reductions[std::min(depth, Limits::max_ply)][std::min(moves_searched, Limits::max_moves - 1)] - (pv_node ? 1 : 0);
        score = -negamax(-alpha - 1, -alpha, depth - 1 - std::clamp(reduction, 1, depth - 2));
      } else
        score = alpha + 1;

      // principal variation search
      if(score > alpha) {
        score = -negamax(-alpha - 1, -alpha, depth - 1);

        if(score > alpha && score < beta)
          score = -negamax(-beta, -alpha, depth - 1);
      }
    }

    --ply_;
    board_.forget_position();
    board_.pop_state();

    if(stopped())
      return 0;

    if(ply_ == 0 && root_count_ < Limits::max_moves) {
      root_moves_[root_count_] = move;
      // fail-soft scores below alpha are bounds; keep them at alpha so the weak skill
      // levels choose among the same near-best moves as before
      root_scores_[root_count_] = std::max(score, alpha);
      ++root_count_;
    }

    ++moves_searched;

    if(score > best_score)
      best_score = score;

    // found a better move
    if(score > alpha) {
      hash_flag = HashFlag::exact;
      best_move = move;
      alpha = score;

      // write PV move and copy the child's line
      pv_table_[ply_][ply_] = move;
      for(int next_ply{ ply_ + 1 }; next_ply < pv_length_[ply_ + 1]; ++next_ply)
        pv_table_[ply_][next_ply] = pv_table_[ply_ + 1][next_ply];
      pv_length_[ply_] = pv_length_[ply_ + 1];

      // fail-soft beta cutoff
      if(score >= beta) {
        engine_.transposition_table.write(board_.state.hash_key, score, depth, HashFlag::beta, ply_, move);

        if(!capture) {
          killer_moves_[1][ply_] = killer_moves_[0][ply_];
          killer_moves_[0][ply_] = move;

          const int bonus = std::min(config.history_bonus_scale * depth * depth, history_limit / 4);
          update_history(move, bonus);
          for(int index{ }; index < quiet_count; ++index)
            update_history(quiet_moves[index], -bonus);
        }

        return score;
      }
    }

    if(!capture)
      quiet_moves[quiet_count++] = move;
  }

  // no legal moves: mate or stalemate
  if(legal_moves == 0)
    return in_check ? -mate_value + ply_ : 0;

  // exact score, or an upper bound at or below alpha when the node fails low
  engine_.transposition_table.write(board_.state.hash_key, best_score, depth, hash_flag, ply_, best_move);
  return best_score;
}

// quiescence search
int Search::quiescence(int alpha, int beta) {
  if(should_return_draw_score())
    return 0;

  if((nodes_ & 2047) == 0)
    communicate();

  ++nodes_;

  if(ply_ > Limits::max_ply - 1)
    return evaluate();

  const int evaluation = evaluate();

  // fail-soft beta cutoff
  if(evaluation >= beta)
    return evaluation;

  int best_score = evaluation;
  if(evaluation > alpha)
    alpha = evaluation;

  MoveList moves_list;
  board_.generate_moves(moves_list, TypeMove::only_captures);
  sort_moves(moves_list);

  const int see_margin = engine_.search_config.quiescence_see_prune_margin;

  for(size_t i{ }; i < moves_list.size(); ++i) {
    const int move = moves_list[i];

    // skip clearly losing exchanges
    const int see = see_evaluate(board_, move);
    if(see < -see_margin && evaluation + see < alpha)
      continue;

    ++ply_;
    board_.remember_position();

    if(!board_.make_move(move, TypeMove::only_captures)) {
      --ply_;
      board_.forget_position();
      continue;
    }

    const int score = -quiescence(-beta, -alpha);

    --ply_;
    board_.forget_position();
    board_.pop_state();

    if(stopped())
      return 0;

    if(score > best_score)
      best_score = score;
    if(score > alpha) {
      alpha = score;
      if(score >= beta)
        return score;
    }
  }

  return best_score;
}

// History with gravity: an entry moves towards +-history_limit by a step that shrinks
// as it gets closer, so it stays bounded and below the killer move scores.
void Search::update_history(const int move, const int bonus) {
  int& entry = history_moves_[Move::get_move_piece(move)][Move::get_move_target(move)];
  entry += bonus - entry * std::abs(bonus) / history_limit;
}

int Search::score_move(const int move, const int hash_move) {
  int promotion_bonus{ };
  if(Move::is_promotion(move))
    promotion_bonus = 2000 + Evaluation::piece_value[Move::get_move_promoted(move)];

  // PV move first
  if(score_pv_ && pv_table_[0][ply_] == move) {
    score_pv_ = false;
    return 20000;
  }

  if(move == hash_move)
    return 30000;

  if(Move::get_move_capture(move)) {
    // en passant victims are not on the target square; they count as pawns
    Pieces target_piece{ P };
    const Pieces start_piece = (board_.state.side == white) ? p : P;
    const Pieces end_piece = (board_.state.side == white) ? k : K;
    const Squares target = Move::get_move_target(move);

    for(Pieces bb_piece{ start_piece }; bb_piece <= end_piece; ++bb_piece) {
      if(get_bit(board_.state.bitboards[bb_piece], target)) {
        target_piece = bb_piece;
        break;
      }
    }

    return mvv_lva[Move::get_move_piece(move)][target_piece] + 10000 + promotion_bonus;
  }

  if(promotion_bonus > 0)
    return 9500 + promotion_bonus;
  if(killer_moves_[0][ply_] == move)
    return 9000;
  if(killer_moves_[1][ply_] == move)
    return 8000;
  return history_moves_[Move::get_move_piece(move)][Move::get_move_target(move)];
}

void Search::sort_moves(MoveList& moves_list, const int hash_move) {
  std::array<int, Limits::max_moves> move_scores;
  const int count = moves_list.count;

  for(int i{ }; i < count; ++i)
    move_scores[i] = score_move(moves_list[i], hash_move);

  // stable insertion sort, best score first
  for(int i{ 1 }; i < count; ++i) {
    const int score = move_scores[i];
    const int move = moves_list[i];
    int j = i;
    while(j > 0 && move_scores[j - 1] < score) {
      move_scores[j] = move_scores[j - 1];
      moves_list[j] = moves_list[j - 1];
      --j;
    }
    move_scores[j] = score;
    moves_list[j] = move;
  }
}

void Search::enable_pv_scoring(const MoveList& moves_list) {
  follow_pv_ = false;

  for(size_t i{ }; i < moves_list.size(); ++i) {
    if(pv_table_[0][ply_] == moves_list[i]) {
      score_pv_ = true;
      follow_pv_ = true;
    }
  }
}

void Search::sort_root_moves() {
  for(int i{ 1 }; i < root_count_; ++i) {
    const int score = root_scores_[i];
    const int move = root_moves_[i];
    int j = i;
    while(j > 0 && root_scores_[j - 1] < score) {
      root_scores_[j] = root_scores_[j - 1];
      root_moves_[j] = root_moves_[j - 1];
      --j;
    }
    root_scores_[j] = score;
    root_moves_[j] = move;
  }
}

// Score of a root move from a fresh shallow search (it shares the hash table and
// move-ordering tables with the main search).
int Search::verify_root_candidate_score(const int move) {
  const int saved_ply = ply_;
  const int saved_repetition_index = board_.repetition_index;
  const bool saved_follow_pv = follow_pv_;
  const bool saved_score_pv = score_pv_;
  int score = -infinity;

  ++ply_;
  board_.remember_position();

  if(board_.make_move(move, TypeMove::all_moves)) {
    follow_pv_ = false;
    score_pv_ = false;
    score = -negamax(-infinity, infinity, 1);
    board_.pop_state();
  }

  ply_ = saved_ply;
  board_.repetition_index = saved_repetition_index;
  follow_pv_ = saved_follow_pv;
  score_pv_ = saved_score_pv;

  return score;
}

bool Search::root_move_repeats_position(const int move) {
  const int saved_ply = ply_;
  const int saved_repetition_index = board_.repetition_index;
  bool repeats{ };

  ++ply_;
  board_.remember_position();

  if(board_.make_move(move, TypeMove::all_moves)) {
    repeats = board_.repetition_count() > 1;
    board_.pop_state();
  }

  ply_ = saved_ply;
  board_.repetition_index = saved_repetition_index;

  return repeats;
}

// Avoids steering into a repetition when a non-repeating move is nearly as good.
int Search::select_non_repeating_root_move(const int best_move) {
  if(!root_move_repeats_position(best_move))
    return best_move;

  const int best_verified_score = verify_root_candidate_score(best_move);
  int fallback_move{ };
  int fallback_verified_score = -infinity;

  for(int index{ }; index < root_count_; ++index) {
    const int candidate_move = root_moves_[index];
    if(candidate_move == best_move || root_move_repeats_position(candidate_move))
      continue;

    const int verified_score = verify_root_candidate_score(candidate_move);
    if(verified_score > fallback_verified_score) {
      fallback_verified_score = verified_score;
      fallback_move = candidate_move;
    }
  }

  if(fallback_move != 0 && fallback_verified_score >= best_verified_score - engine_.search_config.anti_repeat_margin)
    return fallback_move;

  return best_move;
}

// Picks the root move to play: the best one, or for weaker skill levels early in the
// game, a random near-best move that survives a tactical check.
int Search::select_skill_move() {
  if(root_count_ == 0)
    return 0;

  sort_root_moves();

  const int best_move = select_non_repeating_root_move(root_moves_[0]);
  const SearchConfig& config = engine_.search_config;

  if(config.skill_level >= SearchConfig::max_skill)
    return best_move;

  if(board_.repetition_index >= config.opening_variety_plies)
    return best_move;

  const int candidate_limit = std::min(config.weak_move_candidates, root_count_);
  const int best_score = root_scores_[0];
  int eligible_count{ 1 };
  while(eligible_count < candidate_limit && root_scores_[eligible_count] >= best_score - config.weak_move_margin_cp)
    ++eligible_count;

  if(eligible_count <= 1)
    return best_move;

  std::array<int, Limits::max_moves> candidate_moves;
  int candidate_count{ 1 };
  candidate_moves[0] = best_move;

  const int best_verified_score = verify_root_candidate_score(best_move);

  for(int index{ 1 }; index < eligible_count; ++index) {
    const int candidate_move = root_moves_[index];
    if(verify_root_candidate_score(candidate_move) >= best_verified_score - config.tactical_margin)
      candidate_moves[candidate_count++] = candidate_move;
  }

  if(candidate_count <= 1)
    return best_move;

  // the best move is weighted by the skill level
  const unsigned int roll = engine_.random.next_u32();
  const int best_move_bias = config.skill_level + 1;
  const int choice_pool = candidate_count + best_move_bias - 1;
  const int weighted_index = static_cast<int>(roll % static_cast<unsigned int>(choice_pool));
  return candidate_moves[weighted_index < candidate_count ? weighted_index : 0];
}

} // namespace maharajah
