#pragma once

#include "Constants.h"
#include "tables/MagicNumbersTable.h"

#include <array>
#include <cassert>

namespace maharajah {

struct AttackTables {
  static inline std::array<std::array<u64, BoardGeometry::squares>, 2> pawn{ };
  static inline std::array<u64, BoardGeometry::squares> knight{ };
  static inline std::array<u64, BoardGeometry::squares> king{ };
  static inline std::array<u64, BoardGeometry::squares> bishop_masks{ };
  static inline std::array<u64, BoardGeometry::squares> rook_masks{ };
  static inline std::array<std::array<u64, MagicTables::bishop_attacks_count>, BoardGeometry::squares> bishop{ };
  static inline std::array<std::array<u64, MagicTables::rook_attacks_count>, BoardGeometry::squares> rook{ };

  static void init();
};

// Attack mask generation
u64 mask_pawn_attacks(Colors color, Squares square);
u64 mask_knight_attacks(Squares square);
u64 mask_king_attacks(Squares square);
u64 mask_bishop_attacks(Squares square);
u64 mask_rook_attacks(Squares square);
u64 bishop_attacks_on_the_fly(Squares square, u64 block);
u64 rook_attacks_on_the_fly(Squares square, u64 block);

u64 set_occupancy(int index, int bits_in_mask, u64 attack_mask);

inline u64 set_bit(u64& bitboard, const Squares square) {
  // Use bitwise OR to set the bit at the specified square
  return bitboard |= (one << square);
}

inline bool get_bit(const u64& bitboard, const Squares square) {
  return bitboard & (one << square);
}

inline u64 pop_bit(u64& bitboard, const Squares square) {
  return get_bit(bitboard, square) ? bitboard ^= (one << square) : zero;
}

inline int count_bits(u64 bitboard) {
#if defined(_MSC_VER)
  return static_cast<int>(__popcnt64(bitboard));
#else
  return __builtin_popcountll(bitboard);
#endif
}

inline Squares get_ls1b_index(u64 bitboard) {
  assert(bitboard);

#if defined(_MSC_VER)
  unsigned long index;
  _BitScanForward64(&index, bitboard);
  return to_square(static_cast<int>(index));
#else
  return to_square(__builtin_ctzll(bitboard));
#endif
}

inline u64 get_bishop_attacks(const Squares square, u64 occupancy) {
  occupancy &= AttackTables::bishop_masks[square];
  occupancy *= MagicTables::bishop_numbers[square];
  occupancy >>= BoardGeometry::squares - MagicTables::bishop_relevant_bits[square];

  return AttackTables::bishop[square][occupancy];
}

inline u64 get_rook_attacks(const Squares square, u64 occupancy) {
  occupancy &= AttackTables::rook_masks[square];
  occupancy *= MagicTables::rook_numbers[square];
  occupancy >>= BoardGeometry::squares - MagicTables::rook_relevant_bits[square];

  return AttackTables::rook[square][occupancy];
}

inline u64 get_queen_attacks(const Squares square, u64 occupancy) {
  return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

} // namespace maharajah
