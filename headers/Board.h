#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

namespace maharajah {

struct Board {
  BoardState state{};
  BoardState copy_state{};
  Move move{};
  MoveList moves_list{};

  void copy_board() {
    copy_state = state;
  }

  void take_back() {
    state = copy_state;
  }

  [[nodiscard]] bool is_square_attacked(Squares square, Colors side) const;
  void make_move(int move, TypeMove move_flag);
  void update_occupancies();
  void generate_moves();
};

} // namespace maharajah
