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

// Start positions for the network's training data (maharajah_texel gen) only; players
// never get them. generate_custom_position draws one shape of army (5-8 pawns, a compound
// piece, few repeated pieces), so a network trained on it alone misjudges the armies
// players build outside that shape.
//   wide:  both sides variant; each army is generate_custom_position's kind (a quarter)
//          or one of any shape: 0-27 pawns, often heavy on one piece type, compound
//          piece optional.
//   mixed: the orthodox army (castling allowed) against a variant army, half of
//          generate_custom_position's kind, half of any shape; colours random.
// The side to move is random. Returns false if no valid position was found; on success
// board.to_fen() is the start FEN, per-side rules and unmoved pawns included.
enum class TrainingStart { wide, mixed };
bool generate_training_position(Board& board, TrainingStart start, unsigned int seed);

// FEN of a generated custom position ("... - - 0 1 Vv" and the unmoved pawns).
[[nodiscard]] std::string custom_position_fen(const Board& board);

} // namespace maharajah
