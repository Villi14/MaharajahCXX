#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

namespace maharajah {

struct Board {
  BoardState state{};
  BoardState copy_state{};
  MoveList moves_list{};
  
  void copy_board() {
    copy_state = state;
  }

  void take_back() {
    state = copy_state;
  }

  [[nodiscard]] bool is_square_attacked(Squares square, Colors side) const;
  bool make_move(int move, TypeMove type_move);
  void update_occupancies();
  void generate_moves();
};

} // namespace maharajah
