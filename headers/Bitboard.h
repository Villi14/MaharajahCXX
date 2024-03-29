#pragma once

#include "Constants.h"
#include "tables/MagicNumbersTable.h"

#include <array>
#include <cassert>

namespace maharajah {

// Attack tables
extern std::array<std::array<u64, end_bit>, 2> pawn_attacks;
extern std::array<u64, end_bit> knight_attacks;
extern std::array<u64, end_bit> king_attacks;
extern std::array<u64, end_bit> bishop_masks;
extern std::array<u64, end_bit> rook_masks;
extern std::array<std::array<u64, bishop_attacks_count>, end_bit> bishop_attacks;
extern std::array<std::array<u64, rook_attacks_count>, end_bit> rook_attacks;

// Attack mask generation
u64 mask_pawn_attacks(Colors color, Squares square);
u64 mask_knight_attacks(Squares square);
u64 mask_king_attacks(Squares square);
u64 mask_bishop_attacks(Squares square);
u64 mask_rook_attacks(Squares square);
u64 bishop_attacks_on_the_fly(Squares square, u64 block);
u64 rook_attacks_on_the_fly(Squares square, u64 block);

void init_all();

u64 set_occupancy(int index, int bits_in_mask, u64 attack_mask);

/**
 * Sets a bit in a u64 bitboard.
 *
 * @param bitboard The u64 bitboard to set the bit in.
 * @param square The square indicating which bit to set.
 * @return The updated bitboard with the bit set.
 */
inline u64 set_bit(u64& bitboard, const Squares square) {
  // Use bitwise OR to set the bit at the specified square
  return bitboard |= (one << square);
}

/**
 * @brief Retrieves the value of a bit in a u64 bitboard.
 *
 * This function retrieves the value of a bit in a u64 bitboard. The bit is specified by the given square.
 * The result is a boolean indicating whether the bit is set or not.
 *
 * @param bitboard The u64 bitboard to retrieve the bit from.
 * @param square The square indicating which bit to retrieve.
 * @return A boolean indicating whether the bit is set or not.
 */
inline bool get_bit(const u64& bitboard, const Squares square) {
  return bitboard & (one << square);
}

/**
 * @brief Pops a bit from a u64 bitboard.
 *
 * This function pops a bit from a u64 bitboard. The bit is specified by the given square.
 * The result is the value of the bit that was popped, or 0 if the bit was not set.
 * The bitboard is updated to reflect the popped bit.
 *
 * @param bitboard The u64 bitboard to pop the bit from.
 * @param square The square indicating which bit to pop.
 * @return The value of the bit that was popped, or 0 if the bit was not set.
 */
inline u64 pop_bit(u64& bitboard, const Squares square) {
  return get_bit(bitboard, square) ? bitboard ^= (one << square) : zero;
}

/**
 * @brief Count the number of bits set in a u64 bitboard.
 *
 * This function uses the __popcnt64/__builtin_popcountll intrinsic to count the number of bits set in a u64 bitboard.
 * The result is returned as an int.
 *
 * @param bitboard The u64 bitboard to count the bits of.
 * @return The number of bits set in the bitboard.
 */
inline int count_bits(u64 bitboard) {
#if defined(_MSC_VER)
  return static_cast<int>(__popcnt64(bitboard));
#else
  return __builtin_popcountll(bitboard);
#endif
}

/**
 * @brief Returns the index of the least significant bit set in a u64 bitboard.
 *
 * This function takes a u64 bitboard and returns the index of the least significant bit set in the bitboard.
 * The index is returned as a Squares type.
 *
 * @param bitboard The u64 bitboard to retrieve the index from.
 * @return The index of the least significant bit set in the bitboard.
 */
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

/**
 * @brief Returns the bishop attacks for a given square and occupancy.
 *
 * @param square The square to get the bishop attacks for.
 * @param occupancy The occupancy of the board to get the bishop attacks for.
 * @return The bishop attacks for the given square and occupancy.
 *
 * This function takes a square and occupancy as input and returns the bishop attacks for that given square and occupancy.
 * It does this by first applying the bishop mask to the occupancy, then multiplying the occupancy by the bishop magic number,
 * and finally shifting the occupancy to the right by the number of relevant bits for the bishop.
 */
inline u64 get_bishop_attacks(const Squares square, u64 occupancy) {
  occupancy &= bishop_masks[square];
  occupancy *= bishop_magic_numbers_table[square];
  occupancy >>= end_bit - bishop_relevant_bits[square];

  return bishop_attacks[square][occupancy];
}

/**
 * @brief Returns the rook attacks for a given square and occupancy.
 *
 * @param square The square to get the rook attacks for.
 * @param occupancy The occupancy of the board to get the rook attacks for.
 * @return The rook attacks for the given square and occupancy.
 *
 * This function takes a square and occupancy as input and returns the rook attacks for that given square and occupancy.
 * It does this by first applying the rook mask to the occupancy, then multiplying the occupancy by the rook magic number,
 * and finally shifting the occupancy to the right by the number of relevant bits for the rook.
 */
inline u64 get_rook_attacks(const Squares square, u64 occupancy) {
  occupancy &= rook_masks[square];
  occupancy *= rook_magic_numbers_table[square];
  occupancy >>= end_bit - rook_relevant_bits[square];

  return rook_attacks[square][occupancy];
}

/**
 * @brief Returns the queen attacks for a given square and occupancy.
 *
 * @param square The square to get the queen attacks for.
 * @param occupancy The occupancy of the board to get the queen attacks for.
 * @return The queen attacks for the given square and occupancy.
 *
 * This function takes a square and occupancy as input and returns the queen attacks for that given square and occupancy.
 * It does this by simply ORing the bishop and rook attacks for the given square and occupancy.
 */
inline u64 get_queen_attacks(const Squares square, u64 occupancy) {
  return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

} // namespace maharajah
