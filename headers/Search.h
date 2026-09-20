#pragma once

#include "Board.h"

namespace maharajah {

struct SearchResult {
  int best_move{ };
  int score{ };
  int depth{ };
  long nodes{ };
};

class Search {
  public:
  static constexpr int infinity{ 50000 };
  static constexpr int mate_score{ 49000 };

  explicit Search(Board& board)
      : board_(board) { }

  [[nodiscard]] SearchResult run(int depth);

  int negamax(int alpha, int beta, int depth, int ply = 0);

  [[nodiscard]] bool is_draw() const;

  private:
  int quiescence(int alpha, int beta);

  Board& board_;
  long nodes_{ };
  int best_move_{ };
};

} // namespace maharajah
