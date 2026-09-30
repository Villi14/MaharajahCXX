#pragma once

#include "../headers/Game.h"

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

} // namespace maharajah
