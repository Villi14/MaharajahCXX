#pragma once

#include "BoardState.h"
#include "Evaluation.h"

namespace maharajah {

// How often each weight enters the evaluation of one position, white minus black, for
// tools/texel.cpp. White's score before the tempo bonus is
// taper(sum of coefficients[i] * weight[i] + fixed) over both phases' weights.
struct EvalTrace {
  std::array<int, eval_weight_count> coefficients{ };
  // terms that are not tuned (piece safety, king material), the same in both phases
  int fixed{ };
  // 0 = endgame .. Evaluator::phase_range = opening
  int phase{ };
};

struct Evaluator {
  static constexpr int phase_range{ Evaluation::opening_phase_score - Evaluation::endgame_phase_score };

  // Classic evaluation from the side to move's point of view (includes a tempo bonus).
  [[nodiscard]] static int evaluate(const BoardState& state);
  // White's evaluation without the tempo bonus; fills `trace`.
  static int evaluate_white(const BoardState& state, EvalTrace& trace);
  // Sum of the material of the non-pawn, non-king pieces of both sides.
  [[nodiscard]] static int game_phase_score(const BoardState& state);
  // game_phase_score clamped and shifted to 0 (endgame) .. phase_range (opening)
  [[nodiscard]] static int phase(const BoardState& state);
  // Penalty for a piece standing on an attacked square (0 for kings and unattacked pieces).
  [[nodiscard]] static int piece_safety_penalty(const BoardState& state, Pieces piece, Squares square);
};

} // namespace maharajah
