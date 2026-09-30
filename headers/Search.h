#pragma once

#include "Engine.h"

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
      : engine_(engine)
      , board_(engine.board) { }

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

  private:
  static constexpr int full_depth_moves{ 4 };
  static constexpr int reduction_limit{ 3 };
  static constexpr int aspiration_window{ 50 };

  void reset();
  void communicate();
  [[nodiscard]] int evaluate() const;
  [[nodiscard]] bool should_return_draw_score() const;
  [[nodiscard]] int effective_depth(int depth) const;

  [[nodiscard]] int score_move(int move);
  void sort_moves(MoveList& moves_list);
  void enable_pv_scoring(const MoveList& moves_list);

  void sort_root_moves();
  int verify_root_candidate_score(int move);
  bool root_move_repeats_position(int move);
  int select_non_repeating_root_move(int best_move);
  int select_skill_move();

  void print_info(std::ostream& out, int score, int depth, int elapsed) const;

  Engine& engine_;
  Board& board_;

  u64 nodes_{ };
  int ply_{ };
  bool follow_pv_{ };
  bool score_pv_{ };
  std::array<std::array<int, Limits::max_ply + 1>, 2> killer_moves_{ };
  std::array<std::array<int, BoardGeometry::squares>, PieceCount::all> history_moves_{ };
  std::array<int, Limits::max_ply + 1> pv_length_{ };
  std::array<std::array<int, Limits::max_ply + 1>, Limits::max_ply + 1> pv_table_{ };
  int root_count_{ };
  std::array<int, Limits::max_moves> root_moves_{ };
  std::array<int, Limits::max_moves> root_scores_{ };
};

} // namespace maharajah
