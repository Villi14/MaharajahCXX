#pragma once

#include "../headers/Board.h"

namespace maharajah {

class Perft {
  public:
  explicit Perft(Board& board) : board_(board) {}

  static int get_time_ms();
  [[nodiscard]] u64 perft(int depth);
  void perft_divide(int depth);
  void perft_test(int depth);

  private:
  [[nodiscard]] u64 perft_rec(int depth);
  Board& board_;
};

} // namespace maharajah
