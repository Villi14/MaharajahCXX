#pragma once

#include "Constants.h"

namespace maharajah {

struct BoardState {
  Colors side{ white };
  int en_passant{ no_square };
  int castle{};
  std::array<u64, 13> bitboards{};
  std::array<u64, 3> occupancies{};
};

} // namespace maharajah
