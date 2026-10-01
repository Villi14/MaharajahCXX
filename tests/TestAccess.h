#pragma once

#include "../headers/Game.h"
#include "../headers/Search.h"

namespace maharajah {

// Test-only access to internal state.
struct GameTestAccess {
  static Engine& engine(Game& game) {
    return game.engine_;
  }
  static Board& board(Game& game) {
    return game.engine_.board;
  }
  static const Board& board(const Game& game) {
    return game.engine_.board;
  }
};

struct SearchTestAccess {
  static bool root_move_repeats_position(Search& search, const int move) {
    return search.root_move_repeats_position(move);
  }
};

} // namespace maharajah
