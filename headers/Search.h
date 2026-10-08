#pragma once

#include "Engine.h"

#include <cstdint>
#include <memory>
#include <ostream>

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
    continuation_history_ = std::make_unique<ContinuationHistory>();
  }

  // Searches to `depth` plies (or until stopped). Writes UCI "info" lines to `info` if given.
  SearchResult run(int depth, std::ostream* info = nullptr);

  int negamax(int alpha, int beta, int depth);
  int quiescence(int alpha, int beta);

  // Draw by repetition, the fifty/seventy-five move rules or insufficient material.
  [[nodiscard]] bool is_draw() const;
  [[nodiscard]] bool is_insufficient_material() const;

  [[nodiscard]] u64 nodes() const {
    return nodes_;
  }

#ifdef MAHARAJAH_TESTING
  friend struct SearchTestAccess;
#endif

  private:
  static constexpr int full_depth_moves{ 4 };
  static constexpr int reduction_limit{ 3 };
  static constexpr int aspiration_window{ 50 };
  // history scores stay within +-history_limit, below the killer moves (8000, 9000)
  static constexpr int history_limit{ 7000 };
  // a move's piece and target square, the index of the continuation history tables
  static constexpr int piece_squares{ PieceCount::all * BoardGeometry::squares };
  // [piece and target of the move 1 or 2 plies earlier][piece and target of this move]
  using ContinuationHistory = std::array<std::array<std::int16_t, piece_squares>, piece_squares>;

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

  void update_history(int move, int bonus);
  // history of a quiet move: from-to history plus the continuation history of the
  // moves 1 and 2 plies earlier, within +-history_limit
  [[nodiscard]] int quiet_history(int move) const;
  // the move played `back` plies above the current node, 0 for none or a null move
  [[nodiscard]] int previous_move(int back) const;
  // `hash_move` (from the hash table, 0 if none) is searched first
  [[nodiscard]] int score_move(int move, int hash_move);
  void sort_moves(MoveList& moves_list, int hash_move = 0);
  void enable_pv_scoring(const MoveList& moves_list);

  void sort_root_moves();
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
  // static evaluation of each ply's node, no_static_eval when in check
  static constexpr int no_static_eval{ -infinity - 1 };
  std::array<int, Limits::max_ply + 1> static_evals_{ };
  // the move searched from each ply (0 for a null move), for the continuation history
  std::array<int, Limits::max_ply + 1> move_stack_{ };
  std::unique_ptr<ContinuationHistory> continuation_history_;
  std::array<int, Limits::max_ply + 1> pv_length_{ };
  std::array<std::array<int, Limits::max_ply + 1>, Limits::max_ply + 1> pv_table_{ };
  int root_count_{ };
  std::array<int, Limits::max_moves> root_moves_{ };
  std::array<int, Limits::max_moves> root_scores_{ };
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
