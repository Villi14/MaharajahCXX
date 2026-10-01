#pragma once

#include "BoardState.h"
#include "Constants.h"

namespace maharajah {

// xorshift32 generator. Hash keys and the skill-level move choice both draw from
// it, in the same order as MaharajahC, so both engines hash positions identically.
struct Random {
  static constexpr unsigned int seed{ 1804289383 };

  unsigned int state{ seed };

  unsigned int next_u32() {
    unsigned int number = state;
    number ^= number << 0xD;
    number ^= number >> 0x11;
    number ^= number << 0x5;
    state = number;
    return number;
  }

  u64 next_u64() {
    const u64 n1 = static_cast<u64>(next_u32()) & 0xFFFF;
    const u64 n2 = static_cast<u64>(next_u32()) & 0xFFFF;
    const u64 n3 = static_cast<u64>(next_u32()) & 0xFFFF;
    const u64 n4 = static_cast<u64>(next_u32()) & 0xFFFF;
    return n1 | (n2 << 0x10) | (n3 << 0x20) | (n4 << 0x30);
  }
};

struct ZobristKeys {
  std::array<std::array<u64, BoardGeometry::squares>, PieceCount::all> piece{ };
  std::array<u64, BoardGeometry::squares> en_passant{ };
  std::array<u64, 16> castle{ };
  u64 side{ };
  // generator state once every key has been drawn; the engine's random stream continues from here
  unsigned int random_state{ };

  [[nodiscard]] static const ZobristKeys& get();
};

[[nodiscard]] u64 generate_hash_key(const BoardState& state);

} // namespace maharajah
