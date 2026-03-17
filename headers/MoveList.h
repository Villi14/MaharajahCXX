#pragma once

#include <array>
#include <cassert>

namespace maharajah {

constexpr int MAX_MOVES = 256;

struct MoveList {
    std::array<int, MAX_MOVES> moves{};
  int count{};

  void clear() {
    count = 0;
  };

  void add(int move) {
    assert(count < MAX_MOVES);
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
