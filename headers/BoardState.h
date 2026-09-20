#pragma once

#include "Constants.h"

namespace maharajah {

struct BoardState {
  Colors side{ white };
  int en_passant{ no_square };
  int castle{ };
  std::array<u64, 13> bitboards{ };
  std::array<u64, 3> occupancies{ };
  int halfmove{ };

  // Same position for repetition purposes (ignores the halfmove clock).
  [[nodiscard]] bool same_position(const BoardState& other) const {
    return side == other.side && castle == other.castle && en_passant == other.en_passant && bitboards == other.bitboards;
  }
};

} // namespace maharajah
