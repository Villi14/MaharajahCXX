#include "../headers/Search.h"
#include "../headers/Endgame.h"
#include "../headers/Bitboard.h"
#include "../headers/Clock.h"
#include "../headers/Evaluation.h"
#include "../headers/Evaluator.h"
#include "../headers/Move.h"
#include "../headers/See.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <span>
#include <thread>
#include <utility>
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

// Plies a late quiet move is reduced by: from the table, one less in PV nodes, at
// least one and never into quiescence (depth is at least reduction_limit).
int late_move_reduction(const int depth, const int moves_searched, const bool pv_node) {
  const int reduction = late_move_reductions[std::min(depth, Limits::max_ply)][std::min(moves_searched, Limits::max_moves - 1)] - (pv_node ? 1 : 0);
  return std::clamp(reduction, 1, depth - 2);
}

// stable insertion sort of `moves` by their `scores`, best score first
void sort_by_score(const std::span<int> moves, const std::span<int> scores) {
  for(std::size_t i{ 1 }; i < moves.size(); ++i) {
    const int score = scores[i];
    const int move = moves[i];
    std::size_t j = i;
    while(j > 0 && scores[j - 1] < score) {
      scores[j] = scores[j - 1];
      moves[j] = moves[j - 1];
      --j;
    }
    scores[j] = score;
    moves[j] = move;
  }
}

} // namespace

void Search::reset() {
  nodes_ = 0;
  reported_nodes_ = 0;
  if(!helper_)
    engine_.time_control.stopped = false;
  follow_pv_ = false;
  score_pv_ = false;
  ply_ = 0;
  root_repetition_index_ = board_.repetition_index;

  killer_moves_ = { };
  history_moves_ = { };
  pv_table_ = { };
  pv_length_ = { };
  root_moves_ = { };
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

int Search::evaluate() {
  const int score = nnue_ ? evaluate_nnue() : Evaluator::evaluate(board_.state);
  return Endgame::adjust(board_.state, score);
}

int Search::evaluate_nnue() {
  const BoardState& state = board_.state;
  NnueAccumulator& accumulator = (*accumulators_)[ply_];
  if(!accumulator.computed || accumulator.key != state.hash_key) {
    // the parent position is the state saved by the last make_move (or null move)
    const NnueAccumulator* parent = ply_ > 0 ? &(*accumulators_)[ply_ - 1] : nullptr;
    if(parent && board_.ply > 0 && parent->computed && parent->key == board_.history[board_.ply - 1].hash_key)
      nnue_->update(board_.history[board_.ply - 1], state, *parent, accumulator);
    else
      nnue_->refresh(state, accumulator);
  }
  return nnue_->evaluate(accumulator, state.side);
}

int Search::effective_depth(const int depth) const {
  const int cap = engine_.search_config.max_depth_cap;

  if(depth <= 0)
    return 1;
  if(cap == Limits::max_ply || depth < cap)
    return depth;
  return cap;
}

bool Search::should_return_draw_score() const {
  if(ply_ == 0)
    return false;

  const BoardState& state = board_.state;
  return (state.standard_rules && state.halfmove >= 100) || board_.is_search_repetition(root_repetition_index_) || board_.is_insufficient_material();
}

void Search::print_info(std::ostream& out, const int score, const int depth, const int elapsed) const {
  const u64 nodes = nodes_ + engine_.time_control.helper_nodes.load(std::memory_order_relaxed);
  if(const int mate_in = Scores::mate_in_moves(score))
    out << "info score mate " << mate_in;
  else
    out << "info score cp " << score;
  out << " depth " << depth << " nodes " << nodes << " nps " << nodes * 1000 / static_cast<u64>(std::max(elapsed, 1)) << " hashfull "
      << engine_.transposition_table.hashfull() << " time " << elapsed << " pv ";

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

    if(!helper_ && root_moves_.count > 0)
      result.best_move = select_skill_move();

    // the next iteration would most likely not finish before the hard limit
    if(!helper_ && engine_.time_control.soft_stoptime && now_ms() >= engine_.time_control.soft_stoptime)
      break;
  }

  // Stopped before the first iteration finished ("stop" right after "go", a time limit
  // already in the past): a one-ply search that nothing interrupts still gives a move.
  if(!helper_ && !result.best_move) {
    TimeControl& time = engine_.time_control;
    const bool timeset = std::exchange(time.timeset, false);
    const bool poll_input = std::exchange(time.poll_input, false);
    time.stopped = false;

    follow_pv_ = false;
    result.score = negamax(-infinity, infinity, 1);
    result.depth = 1;
    if(root_moves_.count > 0)
      result.best_move = select_skill_move();

    time.timeset = timeset;
    time.poll_input = poll_input;
    time.stopped = true;
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
  // Against a bare king, and a pawn or more up in a small endgame, every level plays at
  // full strength: the weak levels' depth cap and pruning left level 1 unable to mate
  // with king + queen or to promote in KPK (Endgame.h). Swapped for this search only,
  // before any helper thread reads it.
  struct ConfigRestore {
    Engine& engine;
    SearchConfig saved;
    ~ConfigRestore() { engine.search_config = saved; }
  } restore{ engine, engine.search_config };
  if(engine.search_config.skill_level < SearchConfig::max_skill && Endgame::plays_full_strength(engine.board.state)) {
    const int ui_difficulty = engine.search_config.ui_difficulty;
    engine.search_config = SearchConfig::for_skill(SearchConfig::max_skill);
    engine.search_config.ui_difficulty = ui_difficulty;
  }

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

bool Search::make_move(const int move, const TypeMove type_move) {
  ++ply_;
  board_.remember_position();

  if(board_.make_move(move, type_move))
    return true;

  --ply_;
  board_.forget_position();
  return false;
}

void Search::make_null_move() {
  ++ply_;
  board_.remember_position();
  board_.make_null_move();
}

void Search::unmake_move() {
  --ply_;
  board_.forget_position();
  board_.pop_state();
}

// Requires the side to move to have at least two pieces besides king and pawns. With a
// single piece the pass may be the only move that does not lose: in
// kbK5/pp6/1P6/8/8/8/8/R7 w after 1. Ra6 every black move is mated (bxa6 b7#, a
// bishop move Rxa7#), but the pass fails high and Ra6 was never searched.
bool Search::zugzwang_prone() const {
  return count_bits(board_.state.non_pawn_pieces(board_.state.side)) < 2;
}

int Search::null_move_score(const int beta, const int depth) {
  // the reduction grows with depth
  const int reduction = 2 + depth / 6;

  make_null_move();
  const int score = -negamax(-beta, -beta + 1, std::max(0, depth - 1 - reduction));
  unmake_move();

  return score;
}

void Search::update_pv(const int move) {
  pv_table_[ply_][ply_] = move;
  for(int next_ply{ ply_ + 1 }; next_ply < pv_length_[ply_ + 1]; ++next_ply)
    pv_table_[ply_][next_ply] = pv_table_[ply_ + 1][next_ply];
  pv_length_[ply_] = pv_length_[ply_ + 1];
}

void Search::reward_quiet_cutoff(const int move, const int depth, const std::span<const int> failed_quiet_moves) {
  killer_moves_[1][ply_] = killer_moves_[0][ply_];
  killer_moves_[0][ply_] = move;

  const int bonus = std::min(engine_.search_config.history_bonus_scale * depth * depth, history_limit / 4);
  update_history(move, bonus);
  for(const int failed_move : failed_quiet_moves)
    update_history(failed_move, -bonus);
}

// Fail-soft negamax alpha-beta with principal variation search: the first move gets the
// full window, later ones a null window that only proves them no better than alpha.
int Search::negamax(int alpha, int beta, int depth) {
  pv_length_[ply_] = ply_;
  if(ply_ == 0)
    root_moves_.count = 0;

  const Colors side = board_.state.side;
  const u64 own_king = board_.state.bitboards[colored(K, side)];

  if(should_return_draw_score())
    return 0;

  const bool pv_node = beta - alpha > 1;
  const TranspositionTable::Probe hash_entry = engine_.transposition_table.probe(board_.state.hash_key, alpha, beta, depth, ply_);
  if(ply_ && !pv_node && hash_entry.score != TranspositionTable::no_entry)
    return hash_entry.score;

  if(nodes_ % communicate_interval == 0)
    communicate();

  const bool in_check = own_king && board_.is_square_attacked(get_ls1b_index(own_king), opponent(side));

  // In check the search goes on (the check extension below) instead of dropping into
  // quiescence, which would stand pat on a position that may be mate.
  if(depth == 0 && !in_check)
    return quiescence(alpha, beta);

  if(ply_ >= Limits::max_ply)
    return evaluate();

  // the king has been captured
  if(!own_king)
    return -mate_value + ply_;

  ++nodes_;

  // not evaluated in check: nothing below that uses it applies there
  const int static_eval = in_check ? 0 : evaluate();

  // check extension
  if(in_check)
    ++depth;

  const SearchConfig& config = engine_.search_config;
  // the pruning below never applies at the root, in PV nodes or in check
  const bool can_prune = ply_ && !pv_node && !in_check;

  // internal iterative reduction: without a stored move the move ordering is poor,
  // so search shallower; the next iteration finds the move in the table
  if(depth >= 4 && ply_ && !hash_entry.move)
    --depth;

  // Reverse futility pruning: so far above beta that a shallow search would fail high
  // too. Like the null move it assumes that some move keeps the evaluation, so beyond
  // the last two plies it is skipped where zugzwang is likely (Morphy's mate below).
  if(depth <= config.pruning_depth && can_prune && static_eval - config.reverse_futility_margin_per_depth * depth >= beta &&
     (depth <= shallow_pruning_depth || !zugzwang_prone()))
    return static_eval;

  // null-move pruning: if the position still fails high after passing the turn, a real
  // move would too; tried only where the static evaluation already beats beta
  if(depth >= 3 && can_prune && static_eval >= beta && !zugzwang_prone()) {
    const int score = null_move_score(beta, depth);
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

  sort_moves(moves_list, hash_entry.move, true);

  HashFlag hash_flag = HashFlag::alpha;
  int best_move{ };
  int best_score{ -infinity };
  int legal_moves{ };
  int moves_searched{ };
  // non-captures searched without a cutoff, for the history malus
  std::array<int, Limits::max_moves> quiet_moves;
  int quiet_count{ };

  for(const int move : moves_list) {
    const bool capture = Move::get_move_capture(move);
    const bool is_quiet_move = !capture && !Move::is_promotion(move);
    const bool low_depth = depth <= config.pruning_depth && can_prune;
    const bool prunable = low_depth && is_quiet_move;

    // Futility pruning: a quiet move cannot lift a static evaluation this far below
    // alpha. Late move pruning: past this many moves, quiet ones are not searched at all.
    const bool futile = prunable && static_eval + config.futility_margin_per_depth * depth <= alpha;
    const bool late = prunable && moves_searched >= config.late_move_pruning_base + config.late_move_pruning_scale * depth;
    const bool shallow = depth <= shallow_pruning_depth;

    // Futility only after a move has been searched: pruning every move would leave the
    // node without a score, or fall through to the no-legal-moves branch and misreport
    // mate/stalemate.
    if(shallow && ((futile && moves_searched > 0) || late))
      continue;

    // a capture that loses this much material is not worth a search this shallow
    const bool losing = low_depth && capture && moves_searched > 0 && see_evaluate(board_, move) < -losing_capture_margin_per_depth * depth;

    if(!make_move(move))
      continue;

    ++legal_moves;

    // A losing capture, and deeper than the last two plies a futile or late quiet move,
    // is dropped only now that it is known not to give check: there the next iteration
    // would not find a pruned mate, it would stay hidden for several.
    if((losing || (!shallow && moves_searched > 0 && (futile || late))) && !board_.in_check()) {
      unmake_move();
      continue;
    }

    int score;
    if(moves_searched == 0) {
      score = -negamax(-beta, -alpha, depth - 1);
    } else {
      // late move reduction: a late quiet move is first searched shallower; one that
      // is not reduced goes straight to the full-depth search below
      if(moves_searched >= full_depth_moves && depth >= reduction_limit && !in_check && is_quiet_move)
        score = -negamax(-alpha - 1, -alpha, depth - 1 - late_move_reduction(depth, moves_searched, pv_node));
      else
        score = alpha + 1;

      // a move that beats alpha in the null window is searched again, with the full
      // window if it lands inside it
      if(score > alpha) {
        score = -negamax(-alpha - 1, -alpha, depth - 1);

        if(score > alpha && score < beta)
          score = -negamax(-beta, -alpha, depth - 1);
      }
    }

    unmake_move();

    if(stopped())
      return 0;

    // fail-soft scores below alpha are bounds; keep them at alpha so the weak skill
    // levels choose among the same near-best moves as before
    if(ply_ == 0)
      root_moves_.add(move, std::max(score, alpha));

    ++moves_searched;
    best_score = std::max(best_score, score);

    // found a better move
    if(score > alpha) {
      hash_flag = HashFlag::exact;
      best_move = move;
      alpha = score;
      update_pv(move);

      // fail-soft beta cutoff
      if(score >= beta) {
        engine_.transposition_table.write(board_.state.hash_key, score, depth, HashFlag::beta, ply_, move);

        if(!capture)
          reward_quiet_cutoff(move, depth, std::span(quiet_moves).first(static_cast<std::size_t>(quiet_count)));

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

// Captures only, until the position is quiet: the side to move may "stand pat" on the
// static evaluation instead of capturing.
int Search::quiescence(int alpha, int beta) {
  if(should_return_draw_score())
    return 0;

  if(nodes_ % communicate_interval == 0)
    communicate();

  ++nodes_;

  if(ply_ >= Limits::max_ply)
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

  for(const int move : moves_list) {
    // skip clearly losing exchanges
    const int see = see_evaluate(board_, move);
    if(see < -see_margin && evaluation + see < alpha)
      continue;

    if(!make_move(move, TypeMove::only_captures))
      continue;

    const int score = -quiescence(-beta, -alpha);

    unmake_move();

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

int Search::score_move(const int move, const int hash_move, const bool see_order) {
  int promotion_bonus{ };
  if(Move::is_promotion(move))
    promotion_bonus = MoveOrder::promotion_bonus + Evaluation::piece_value[Move::get_move_promoted(move)];

  // checked before the hash move, though it scores below it; only the first match counts
  if(score_pv_ && pv_table_[0][ply_] == move) {
    score_pv_ = false;
    return MoveOrder::pv_move;
  }

  if(move == hash_move)
    return MoveOrder::hash_move;

  if(Move::get_move_capture(move)) {
    // en passant victims are not on the target square; they count as pawns
    const Pieces victim = board_.state.piece_on(Move::get_move_target(move), opponent(board_.state.side));
    const int piece = Move::get_move_piece(move);
    const int order = mvv_lva[piece][victim == no_pieces ? P : victim] + promotion_bonus;
    // only a more valuable piece taking a less valuable one can lose material
    const bool may_lose = victim != no_pieces && Evaluation::piece_value[piece] > Evaluation::piece_value[victim];
    if(see_order && may_lose && !promotion_bonus && see_evaluate(board_, move) < 0)
      return MoveOrder::losing_capture + order;
    return MoveOrder::capture + order;
  }

  if(promotion_bonus > 0)
    return MoveOrder::quiet_promotion + promotion_bonus;
  if(killer_moves_[0][ply_] == move)
    return MoveOrder::first_killer;
  if(killer_moves_[1][ply_] == move)
    return MoveOrder::second_killer;
  return history_moves_[Move::get_move_piece(move)][Move::get_move_target(move)];
}

void Search::sort_moves(MoveList& moves_list, const int hash_move, const bool see_order) {
  std::array<int, Limits::max_moves> move_scores;
  const std::size_t count = moves_list.size();

  for(std::size_t i{ }; i < count; ++i)
    move_scores[i] = score_move(moves_list[i], hash_move, see_order);

  sort_by_score(std::span(moves_list.moves).first(count), std::span(move_scores).first(count));
}

void Search::enable_pv_scoring(const MoveList& moves_list) {
  follow_pv_ = false;

  for(const int move : moves_list) {
    if(pv_table_[0][ply_] == move) {
      score_pv_ = true;
      follow_pv_ = true;
    }
  }
}

void Search::RootMoves::sort() {
  const auto size = static_cast<std::size_t>(count);
  sort_by_score(std::span(moves).first(size), std::span(scores).first(size));
}

// Score of a root move from a fresh shallow search (it shares the hash table and
// move-ordering tables with the main search).
int Search::verify_root_candidate_score(const int move) {
  const bool saved_follow_pv = follow_pv_;
  const bool saved_score_pv = score_pv_;
  int score = -infinity;

  if(make_move(move)) {
    follow_pv_ = false;
    score_pv_ = false;
    score = -negamax(-infinity, infinity, 1);
    unmake_move();
  }

  follow_pv_ = saved_follow_pv;
  score_pv_ = saved_score_pv;

  return score;
}

bool Search::root_move_repeats_position(const int move) {
  bool repeats{ };

  if(make_move(move)) {
    repeats = board_.repetition_count() > 1;
    unmake_move();
  }

  return repeats;
}

// Avoids steering into a repetition when a non-repeating move is nearly as good.
int Search::select_non_repeating_root_move(const int best_move) {
  if(!root_move_repeats_position(best_move))
    return best_move;

  const int best_verified_score = verify_root_candidate_score(best_move);
  int fallback_move{ };
  int fallback_verified_score = -infinity;

  for(int index{ }; index < root_moves_.count; ++index) {
    const int candidate_move = root_moves_.moves[index];
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
  if(root_moves_.count == 0)
    return 0;

  root_moves_.sort();

  const int best_move = select_non_repeating_root_move(root_moves_.moves[0]);
  const SearchConfig& config = engine_.search_config;

  if(config.skill_level >= SearchConfig::max_skill)
    return best_move;

  if(board_.repetition_index >= config.opening_variety_plies)
    return best_move;

  // the moves scoring within the margin of the best one, among the first candidates
  const int candidate_limit = std::min(config.weak_move_candidates, root_moves_.count);
  const int best_score = root_moves_.scores[0];
  int eligible_count{ 1 };
  while(eligible_count < candidate_limit && root_moves_.scores[eligible_count] >= best_score - config.weak_move_margin_cp)
    ++eligible_count;

  if(eligible_count <= 1)
    return best_move;

  std::array<int, Limits::max_moves> candidate_moves;
  int candidate_count{ 1 };
  candidate_moves[0] = best_move;

  const int best_verified_score = verify_root_candidate_score(best_move);

  // of those, the ones that hold up in a fresh search
  for(int index{ 1 }; index < eligible_count; ++index) {
    const int candidate_move = root_moves_.moves[index];
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
