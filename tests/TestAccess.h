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
  // the pseudo-legal moves of the search's position in the order they are searched
  static MoveList sorted_moves(Search& search, const bool see_order) {
    MoveList moves;
    search.board_.generate_moves(moves);
    search.sort_moves(moves, 0, see_order);
    return moves;
  }
};

} // namespace maharajah
