#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

#include <cassert>

namespace maharajah {

struct Board {
  int ply{};
  int best_move{};
  BoardState state{};
  MoveList moves_list{};
  std::array<BoardState, max_ply> history{};

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
