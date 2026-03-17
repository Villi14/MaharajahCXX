#pragma once

#include <cstddef>
#include <cassert>

namespace maharajah {

constexpr int MAX_MOVES = 256;

struct MoveList {
  int moves[MAX_MOVES];
  int count;
  void clear() { count = 0; }
  void add(int move) {
    assert(count < MAX_MOVES);
    moves[count++] = move;
  }
  std::size_t size() const { return static_cast<std::size_t>(count); }
  int operator[](int idx) const { return moves[idx]; }
  int& operator[](int idx) { return moves[idx]; }
};

} // namespace maharajah
