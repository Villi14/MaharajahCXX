#pragma once

#include "BoardState.h"
#include "Evaluation.h"

namespace maharajah {
struct Evaluator {
  [[nodiscard]] static int evaluate(const BoardState& state);
};

} // namespace maharajah
