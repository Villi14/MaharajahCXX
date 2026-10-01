#pragma once

#include "Board.h"

#include <string>

namespace maharajah {

// Generates a reasonable custom Maharajah start: kings on e1/e8 and a 39-point army per
// side on its own half, played under variant rules on both sides.
// side_to_move: 0 = white, 1 = black, anything else = random. Uses std::rand seeded
// with `seed`, so a seed reproduces the same position. Returns false if no valid
// position was found.
bool generate_custom_position(Board& board, int side_to_move, unsigned int seed);

// FEN of a generated custom position ("... - - 0 1 Vv" and the unmoved pawns).
[[nodiscard]] std::string custom_position_fen(const Board& board);

} // namespace maharajah
