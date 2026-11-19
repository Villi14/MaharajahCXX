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
// Each start fixes the kind of game: mixed is always the orthodox army against a variant
// one, wide / any always variant against variant, so the gen shares set the mix of
// kinds exactly (tools/nnue/data_mix.py measures it in the data).
//   mixed: the orthodox army (castling allowed) against a balanced variant army (below);
//          colours random.
//   any:   two balanced variant armies, or one and its mirror image (a quarter).
//          Balanced: any of the 2580 armies a player can build (39 points, at most 31
//          men) half the time, half of that evenly and half a type and its number
//          first (1-3 maharajahs, 1-4 queens, 0-30 pawns, ...); a wall army a fifth;
//          a wide army or a players' army (generate_custom_position's kind) the rest.
//   wide:  two variant armies; each generate_custom_position's kind (a quarter) or one
//          of any shape: 0-27 pawns, often heavy on one piece type, compound optional.
//   wall:  a pawn wall (1-3 solid ranks, 8-27 pawns) with a swarm of one type behind it
//          (knights, bishops, both, rooks or queens) against the orthodox army, another
//          wall army or a wide / players' army, a third each.
// Variant armies are placed by the players' rule: ranks filled from the home rank out.
// The side to move is random. Returns false if no valid position was found; on success
// board.to_fen() is the start FEN, per-side rules and unmoved pawns included.
enum class TrainingStart { wide, mixed, wall, any };
bool generate_training_position(Board& board, TrainingStart start, unsigned int seed);

// FEN of a generated custom position ("... - - 0 1 Vv" and the unmoved pawns).
[[nodiscard]] std::string custom_position_fen(const Board& board);

} // namespace maharajah
