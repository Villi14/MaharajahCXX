#pragma once

#include "../headers/Game.h"

namespace maharajah {

// Test-only access to internal state.
struct GameTestAccess {
  static Board& board(Game& game) {
    return game.board_;
  }
  static const Board& board(const Game& game) {
    return game.board_;
  }
};

} // namespace maharajah
