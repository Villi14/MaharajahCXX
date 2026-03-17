#pragma once

#include "Constants.h"

namespace maharajah {

struct BoardState {
  Colors side{ white };
  int en_passant{ no_square };
  int castle{};
  u64 bitboards[13];
  u64 occupancies[3];
};

} // namespace maharajah
