#pragma once

#include "Board.h"

namespace maharajah {

// Static exchange evaluation of a capture from the side to move's point of view
// (centipawns); 0 for quiet or illegal moves. The board is left unchanged.
[[nodiscard]] int see_evaluate(Board& board, int move);

} // namespace maharajah
