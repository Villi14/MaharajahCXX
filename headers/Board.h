#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

#include <cassert>

namespace maharajah {

struct Board {
  BoardState state{};
  std::array<BoardState, max_ply> history{};
  int ply{};
  MoveList moves_list{};

  void push_state() {
    assert(ply < max_ply);
    history[ply++] = state;
  }

  void pop_state() {
    assert(ply > 0);
    state = history[--ply];
  }

  [[nodiscard]] bool is_square_attacked(Squares square, Colors side) const;
  bool make_move(int move, TypeMove type_move);
  void update_occupancies();
  void generate_moves();
};

} // namespace maharajah
