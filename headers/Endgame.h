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

// at most this many pieces and pawns besides the kings make a small endgame
inline constexpr int small_endgame_men{ 10 };
// and the side to move is at least this much material up (Evaluation::piece_value)
inline constexpr int small_endgame_lead{ 82 };

// The side to move should convert at full strength: a bare-king win, or a small
// endgame in which it is at least a pawn up. The side that is behind keeps its
// level, so a player who is ahead can still convert against it.
// Measured 2026-11-09 on 358 won endgames from self-play (Lichess tablebase for
// <= 5 men, else a +200 search), side to move at level 1 against level 5, 50 ms:
// 1.3.0 won 49 %, <= 6 men 77 %, <= 8 men 89 %, <= 10 men 96 % (level 3: 80 % ->
// 96 %); a two-pawn lead instead of one converted less. The levels get stronger by
// about +50 Elo (level 1) and +35 (level 3), far less than the gap between levels.
[[nodiscard]] bool plays_full_strength(const BoardState& state);

// `score` is from the side to move's point of view; so is the result.
[[nodiscard]] int adjust(const BoardState& state, int score);

} // namespace Endgame

} // namespace maharajah
