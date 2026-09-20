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
  };

  void add(int move) {
    assert(count < Limits::max_moves);
    moves[count++] = move;
  }

  [[nodiscard]] std::size_t size() const {
    return count;
  }

  int operator[](const int index) const {
    return moves[index];
  }

  int& operator[](const int index) {
    return moves[index];
  }

  int operator[](const std::size_t index) const {
    return moves[index];
  }

  int& operator[](const std::size_t index) {
    return moves[index];
  }
};

} // namespace maharajah
