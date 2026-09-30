#pragma once

#include "BoardState.h"
#include "EngineConfig.h"
#include "Evaluation.h"

namespace maharajah {

struct Evaluator {
  // Classic evaluation from the side to move's point of view (includes a tempo bonus).
  [[nodiscard]] static int evaluate(const BoardState& state, const EvalConfig& config = EvalConfig{ });
  // Sum of the opening material of the non-pawn, non-king pieces of both sides.
  [[nodiscard]] static int game_phase_score(const BoardState& state);
  // Penalty for a piece standing on an attacked square (0 for kings and unattacked pieces).
  [[nodiscard]] static int piece_safety_penalty(const BoardState& state, Pieces piece, Squares square);
};

} // namespace maharajah
