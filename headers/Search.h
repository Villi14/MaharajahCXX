#pragma once

#include "Engine.h"

#include <memory>
#include <ostream>
#include <span>

namespace maharajah {

struct SearchResult {
  int best_move{ };
  // score of the last completed iteration
  int score{ };
  // last completed iteration
  int depth{ };
  u64 nodes{ };
};

// Iterative-deepening PVS with aspiration windows, a transposition table, null-move
// pruning, late-move reductions, futility / late-move pruning and SEE-pruned quiescence.
// Weaker skill levels cap the depth and may pick a near-best root move.
class Search {
  public:
  static constexpr int infinity{ Scores::infinity };
  static constexpr int mate_value{ Scores::mate_value };
  static constexpr int mate_score{ Scores::mate_score };

  explicit Search(Engine& engine)
      : Search(engine, engine.board) { }

  // Searches `board` (a copy of the engine's, for a helper thread) with the engine's
  // hash table and settings. Thread 0 is the main search: it alone checks the clock
  // and input, clears the stop flag and picks the move. A helper just stops when the
  // flag is set.
  Search(Engine& engine, Board& board, const int thread_index = 0)
      : engine_(engine)
      , board_(board)
      , thread_index_(thread_index)
      , helper_(thread_index > 0) {
    if(engine.eval_config.eval_mode == EvalMode::nnue && engine.nnue.has_loaded_weights()) {
      nnue_ = &engine.nnue;
      accumulators_ = std::make_unique<std::array<NnueAccumulator, Limits::max_ply + 1>>();
    }
  }

  // Searches to `depth` plies (or until stopped). Writes UCI "info" lines to `info` if given.
  SearchResult run(int depth, std::ostream* info = nullptr);

  int negamax(int alpha, int beta, int depth);
  int quiescence(int alpha, int beta);

  [[nodiscard]] u64 nodes() const {
    return nodes_;
  }

#ifdef MAHARAJAH_TESTING
  friend struct SearchTestAccess;
#endif

  private:
  // late move reductions start with this move, from this depth on
  static constexpr int full_depth_moves{ 4 };
  static constexpr int reduction_limit{ 3 };
  static constexpr int aspiration_window{ 50 };
  // the clock and input are checked once per this many nodes
  static constexpr u64 communicate_interval{ 2048 };

  // Move-ordering scores; moves are searched highest score first.
  struct MoveOrder {
    static constexpr int hash_move{ 30000 };
    // the previous iteration's principal variation move
    static constexpr int pv_move{ 20000 };
    // plus MVV-LVA and, for a capturing promotion, the promotion bonus
    static constexpr int capture{ 10000 };
    // plus the promotion bonus
    static constexpr int quiet_promotion{ 9500 };
    // plus the value of the promoted piece
    static constexpr int promotion_bonus{ 2000 };
    static constexpr int first_killer{ 9000 };
    static constexpr int second_killer{ 8000 };
  };
  // history scores stay within +-history_limit, below the killer moves
  static constexpr int history_limit{ 7000 };
  static_assert(history_limit < MoveOrder::second_killer);

  // The root moves of the current iteration with their scores, for picking the move
  // to play (anti-repetition, weaker skill levels).
  struct RootMoves {
    int count{ };
    std::array<int, Limits::max_moves> moves{ };
    std::array<int, Limits::max_moves> scores{ };

    void add(const int move, const int score) {
      if(count == Limits::max_moves)
        return;
      moves[count] = move;
      scores[count] = score;
      ++count;
    }

    // best score first, ties in search order
    void sort();
  };

  void reset();
  void communicate();
  // adds the nodes searched since the last report to the engine's helper_nodes
  void report_nodes();
  [[nodiscard]] bool stopped() const {
    return engine_.time_control.stopped.load(std::memory_order_relaxed);
  }
  [[nodiscard]] int evaluate();
  // NNUE evaluation of the current node: its accumulator is reused, updated from the
  // parent's or rebuilt
  [[nodiscard]] int evaluate_nnue();
  [[nodiscard]] bool should_return_draw_score() const;
  [[nodiscard]] int effective_depth(int depth) const;

  // Plays `move` one ply deeper, remembering the position for repetition detection;
  // false (and nothing changed) if the move is illegal.
  bool make_move(int move, TypeMove type_move = TypeMove::all_moves);
  // Passes the turn one ply deeper (null-move pruning).
  void make_null_move();
  // Takes back the last make_move or make_null_move.
  void unmake_move();

  // true when the side to move may be in zugzwang, where null-move pruning is unsound
  [[nodiscard]] bool zugzwang_prone() const;
  // score after passing the turn, from a reduced null-window search at beta
  int null_move_score(int beta, int depth);
  // makes `move` followed by the child's line the principal variation from this ply
  void update_pv(int move);
  // killer moves and history after the quiet `move` caused a beta cutoff, with a
  // malus for the quiet moves searched before it
  void reward_quiet_cutoff(int move, int depth, std::span<const int> failed_quiet_moves);

  void update_history(int move, int bonus);
  // `hash_move` (from the hash table, 0 if none) is searched first
  [[nodiscard]] int score_move(int move, int hash_move);
  void sort_moves(MoveList& moves_list, int hash_move = 0);
  void enable_pv_scoring(const MoveList& moves_list);

  int verify_root_candidate_score(int move);
  bool root_move_repeats_position(int move);
  int select_non_repeating_root_move(int best_move);
  int select_skill_move();

  void print_info(std::ostream& out, int score, int depth, int elapsed) const;

  Engine& engine_;
  Board& board_;
  int thread_index_{ };
  bool helper_{ };
  // nodes already added to the engine's helper_nodes
  u64 reported_nodes_{ };

  u64 nodes_{ };
  int ply_{ };
  bool follow_pv_{ };
  bool score_pv_{ };
  std::array<std::array<int, Limits::max_ply + 1>, 2> killer_moves_{ };
  std::array<std::array<int, BoardGeometry::squares>, PieceCount::all> history_moves_{ };
  std::array<int, Limits::max_ply + 1> pv_length_{ };
  std::array<std::array<int, Limits::max_ply + 1>, Limits::max_ply + 1> pv_table_{ };
  RootMoves root_moves_{ };
  // set when the engine evaluates with a loaded network; accumulators by ply
  const Nnue* nnue_{ };
  std::unique_ptr<std::array<NnueAccumulator, Limits::max_ply + 1>> accumulators_;
};

// Lazy SMP: searches the engine's position with `engine.threads` threads that share
// the hash table, each on its own copy of the board. The main thread reports and
// picks the move; the helpers only fill the hash table, until the main thread
// finishes. With one thread this is Search(engine).run(); with more the result
// depends on thread timing.
SearchResult run_search(Engine& engine, int depth, std::ostream* info = nullptr);

} // namespace maharajah
