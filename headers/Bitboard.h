#pragma once

#include "Constants.h"
#include "tables/MagicNumbersTable.h"

#include <array>
#include <bit>
#include <cassert>

namespace maharajah {

// Precomputed attack sets, indexed by square (sliders by square and magic index).
struct AttackTables {
  std::array<std::array<u64, BoardGeometry::squares>, 2> pawn{ };
  std::array<u64, BoardGeometry::squares> knight{ };
  std::array<u64, BoardGeometry::squares> king{ };
  std::array<u64, BoardGeometry::squares> bishop_masks{ };
  std::array<u64, BoardGeometry::squares> rook_masks{ };
  std::array<std::array<u64, MagicTables::bishop_attacks_count>, BoardGeometry::squares> bishop{ };
  std::array<std::array<u64, MagicTables::rook_attacks_count>, BoardGeometry::squares> rook{ };

  // fills the tables in place: at 2.3 MB they must never live on the stack
  AttackTables();
};

// The only instance, built before main() and read-only afterwards.
extern const AttackTables attack_tables;

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

inline int count_bits(const u64 bitboard) {
  return std::popcount(bitboard);
}

inline Squares get_ls1b_index(const u64 bitboard) {
  assert(bitboard);
  return to_square(std::countr_zero(bitboard));
}

// square of the least significant bit, which is cleared
inline Squares pop_ls1b(u64& bitboard) {
  const Squares square = get_ls1b_index(bitboard);
  bitboard &= bitboard - 1;
  return square;
}

inline u64 get_bishop_attacks(const Squares square, u64 occupancy) {
  occupancy &= attack_tables.bishop_masks[square];
  occupancy *= MagicTables::bishop_numbers[square];
  occupancy >>= BoardGeometry::squares - MagicTables::bishop_relevant_bits[square];

  return attack_tables.bishop[square][occupancy];
}

inline u64 get_rook_attacks(const Squares square, u64 occupancy) {
  occupancy &= attack_tables.rook_masks[square];
  occupancy *= MagicTables::rook_numbers[square];
  occupancy >>= BoardGeometry::squares - MagicTables::rook_relevant_bits[square];

  return attack_tables.rook[square][occupancy];
}

inline u64 get_queen_attacks(const Squares square, u64 occupancy) {
  return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

// bishop + knight
inline u64 get_archbishop_attacks(const Squares square, u64 occupancy) {
  return get_bishop_attacks(square, occupancy) | attack_tables.knight[square];
}

// rook + knight
inline u64 get_chancellor_attacks(const Squares square, u64 occupancy) {
  return get_rook_attacks(square, occupancy) | attack_tables.knight[square];
}

// queen + knight
inline u64 get_amazon_attacks(const Squares square, u64 occupancy) {
  return get_queen_attacks(square, occupancy) | attack_tables.knight[square];
}

// Squares attacked from `square` by a non-pawn piece of the given kind.
inline u64 get_piece_attacks(const Pieces piece, const Squares square, const u64 occupancy) {
  switch(piece) {
  case N:
  case n:
    return attack_tables.knight[square];
  case B:
  case b:
    return get_bishop_attacks(square, occupancy);
  case R:
  case r:
    return get_rook_attacks(square, occupancy);
  case Q:
  case q:
    return get_queen_attacks(square, occupancy);
  case A:
  case a:
    return get_archbishop_attacks(square, occupancy);
  case C:
  case c:
    return get_chancellor_attacks(square, occupancy);
  case M:
  case m:
    return get_amazon_attacks(square, occupancy);
  case K:
  case k:
    return attack_tables.king[square];
  default:
    return zero;
  }
}

// One side's pieces grouped by how they attack a square. Compound pieces attack like
// their components, so an attack test computes each attack set (knight jumps,
// diagonals, lines) once for all the pieces moving that way. The knight, diagonal and
// line squares around a square never overlap.
struct AttackerGroups {
  u64 pawns{ };
  // N, A, C, M
  u64 knight_movers{ };
  // B, A, Q, M
  u64 diagonal_movers{ };
  // R, C, Q, M
  u64 line_movers{ };
  u64 king{ };

  AttackerGroups(const std::array<u64, PieceCount::all>& bitboards, const Colors side) {
    const u64 archbishops = bitboards[colored(A, side)];
    const u64 chancellors = bitboards[colored(C, side)];
    const u64 queens = bitboards[colored(Q, side)];
    const u64 amazons = bitboards[colored(M, side)];

    pawns = bitboards[colored(P, side)];
    knight_movers = bitboards[colored(N, side)] | archbishops | chancellors | amazons;
    diagonal_movers = bitboards[colored(B, side)] | archbishops | queens | amazons;
    line_movers = bitboards[colored(R, side)] | chancellors | queens | amazons;
    king = bitboards[colored(K, side)];
  }
};

} // namespace maharajah
