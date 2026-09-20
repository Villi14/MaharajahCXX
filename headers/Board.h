#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

#include <cassert>

namespace maharajah {

struct Board {
  int ply{ };
  BoardState state{ };
  MoveList moves_list{ };
  std::array<BoardState, Limits::max_ply> history{ };

  void push_state() {
    assert(ply < Limits::max_ply);
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
  void generate_moves(MoveList& moves_list) const;
};

} // namespace maharajah
