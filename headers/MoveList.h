#pragma once

#include "Constants.h"
#include <array>
#include <cassert>

namespace maharajah {

struct MoveList {
  int count{ };
  std::array<int, Limits::max_moves> moves{ };

  void clear() {
    count = 0;
  }

  void add(int move) {
    assert(count < Limits::max_moves);
    moves[count++] = move;
  }

  [[nodiscard]] std::size_t size() const {
    return count;
  }

  int operator[](const std::size_t index) const {
    return moves[index];
  }

  int& operator[](const std::size_t index) {
    return moves[index];
  }

  // the generated moves, for range-for
  [[nodiscard]] const int* begin() const {
    return moves.data();
  }

  [[nodiscard]] const int* end() const {
    return moves.data() + count;
  }
};

} // namespace maharajah
