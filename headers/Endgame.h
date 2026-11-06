#pragma once

#include "BoardState.h"

namespace maharajah {

// Endgame knowledge applied on top of the network (or the classic evaluation).
//
// The network is trained on self-play that is adjudicated as won at +1000 and stops
// there, so it has hardly seen won endgames played out: with rook + knight + pawn
// against a bare king it scored the king move higher than promoting the pawn, and at
// a capped depth the engine shuffled for hundreds of moves. It also scores known
// draws (KNNK, KR v KB, the wrong bishop) as big wins.
//
// - Bare king: when one side has only its king and the other has mating material
//   without help from its pawns, the score is a known win plus the material, the
//   pawns' advance and a mop-up term (defending king to the edge, or to the corner of
//   the bishop's colour with bishop + knight; kings close). Every promotion and every
//   step towards mate raises it, so even a depth-limited search makes progress.
// - Drawish material: the stronger side has no pawns and is no more than a bishop up,
//   or has only two knights, or a bishop with rook pawns whose promotion square is
//   the other colour and is held by the defending king: the score is scaled down.
namespace Endgame {

// a bare-king win scores at least this (far below Scores::mate_score)
inline constexpr int known_win{ 10000 };

// One side has only its king and the other can force mate without promoting. The
// search then ignores the difficulty's depth cap: a weak level still has to be able
// to mate a bare king (level 1 never mated with king + queen).
[[nodiscard]] bool is_bare_king_win(const BoardState& state);

// `score` is from the side to move's point of view; so is the result.
[[nodiscard]] int adjust(const BoardState& state, int score);

} // namespace Endgame

} // namespace maharajah
