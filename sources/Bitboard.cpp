#include "../headers/Bitboard.h"
#include "../headers/tables/MagicNumbersTable.h"

using std::array;

namespace maharajah {

u64 mask_pawn_attacks(const Colors color, const Squares square) {
  u64 attacks{ };
  u64 bitboard{ };

  set_bit(bitboard, square);

  if(color == white) {
    if((bitboard >> 7) & FileMask::not_a_file)
      attacks |= (bitboard >> 7);
    if((bitboard >> 9) & FileMask::not_h_file)
      attacks |= (bitboard >> 9);
  } else {
    if((bitboard << 7) & FileMask::not_h_file)
      attacks |= (bitboard << 7);
    if((bitboard << 9) & FileMask::not_a_file)
      attacks |= (bitboard << 9);
  }

  return attacks;
}

u64 mask_knight_attacks(const Squares square) {
  u64 attacks{ };
  u64 bitboard{ };

  set_bit(bitboard, square);

  if((bitboard >> 17) & FileMask::not_h_file)
    attacks |= (bitboard >> 17);
  if((bitboard >> 15) & FileMask::not_a_file)
    attacks |= (bitboard >> 15);
  if((bitboard >> 10) & FileMask::not_gh_file)
    attacks |= (bitboard >> 10);
  if((bitboard >> 6) & FileMask::not_ab_file)
    attacks |= (bitboard >> 6);
  if((bitboard << 17) & FileMask::not_a_file)
    attacks |= (bitboard << 17);
  if((bitboard << 15) & FileMask::not_h_file)
    attacks |= (bitboard << 15);
  if((bitboard << 10) & FileMask::not_ab_file)
    attacks |= (bitboard << 10);
  if((bitboard << 6) & FileMask::not_gh_file)
    attacks |= (bitboard << 6);

  return attacks;
}

u64 mask_king_attacks(const Squares square) {
  u64 attacks{ };
  u64 bitboard{ };

  set_bit(bitboard, square);

  if(bitboard >> 8)
    attacks |= (bitboard >> 8);
  if((bitboard >> 9) & FileMask::not_h_file)
    attacks |= (bitboard >> 9);
  if((bitboard >> 7) & FileMask::not_a_file)
    attacks |= (bitboard >> 7);
  if((bitboard >> 1) & FileMask::not_h_file)
    attacks |= (bitboard >> 1);
  if(bitboard << 8)
    attacks |= (bitboard << 8);
  if((bitboard << 9) & FileMask::not_a_file)
    attacks |= (bitboard << 9);
  if((bitboard << 7) & FileMask::not_h_file)
    attacks |= (bitboard << 7);
  if((bitboard << 1) & FileMask::not_a_file)
    attacks |= (bitboard << 1);

  return attacks;
}

u64 mask_bishop_attacks(const Squares square) {
  u64 attacks{ };
  int rank{ }, file{ };
  const int target_rank = square / BoardGeometry::ranks;
  const int target_file = square % BoardGeometry::files;

  for(rank = target_rank + 1, file = target_file + 1; rank <= 6 && file <= 6; ++rank, ++file)
    attacks |= (one << (rank * BoardGeometry::files + file));
  for(rank = target_rank - 1, file = target_file + 1; rank >= 1 && file <= 6; --rank, ++file)
    attacks |= (one << (rank * BoardGeometry::files + file));
  for(rank = target_rank + 1, file = target_file - 1; rank <= 6 && file >= 1; ++rank, --file)
    attacks |= (one << (rank * BoardGeometry::files + file));
  for(rank = target_rank - 1, file = target_file - 1; rank >= 1 && file >= 1; --rank, --file)
    attacks |= (one << (rank * BoardGeometry::files + file));

  return attacks;
}

u64 mask_rook_attacks(const Squares square) {
  u64 attacks{ };
  int rank{ }, file{ };
  const int target_rank = square / BoardGeometry::ranks;
  const int target_file = square % BoardGeometry::files;

  for(rank = target_rank + 1; rank <= 6; ++rank)
    attacks |= (one << (rank * BoardGeometry::files + target_file));
  for(rank = target_rank - 1; rank >= 1; --rank)
    attacks |= (one << (rank * BoardGeometry::files + target_file));
  for(file = target_file + 1; file <= 6; ++file)
    attacks |= (one << (target_rank * BoardGeometry::files + file));
  for(file = target_file - 1; file >= 1; --file)
    attacks |= (one << (target_rank * BoardGeometry::files + file));

  return attacks;
}

u64 bishop_attacks_on_the_fly(const Squares square, const u64 block) {
  u64 attacks{ };
  int rank{ }, file{ };
  const int target_rank = square / BoardGeometry::ranks;
  const int target_file = square % BoardGeometry::files;

  for(rank = target_rank + 1, file = target_file + 1; rank <= 7 && file <= 7; ++rank, ++file) {
    attacks |= (one << (rank * BoardGeometry::files + file));
    if((one << (rank * BoardGeometry::files + file)) & block)
      break;
  }

  for(rank = target_rank - 1, file = target_file + 1; rank >= 0 && file <= 7; --rank, ++file) {
    attacks |= (one << (rank * BoardGeometry::files + file));
    if((one << (rank * BoardGeometry::files + file)) & block)
      break;
  }

  for(rank = target_rank + 1, file = target_file - 1; rank <= 7 && file >= 0; ++rank, --file) {
    attacks |= (one << (rank * BoardGeometry::files + file));
    if((one << (rank * BoardGeometry::files + file)) & block)
      break;
  }

  for(rank = target_rank - 1, file = target_file - 1; rank >= 0 && file >= 0; --rank, --file) {
    attacks |= (one << (rank * BoardGeometry::files + file));
    if((one << (rank * BoardGeometry::files + file)) & block)
      break;
  }

  return attacks;
}

u64 rook_attacks_on_the_fly(const Squares square, const u64 block) {
  u64 attacks{ };
  int rank{ }, file{ };
  const int target_rank = square / BoardGeometry::ranks;
  const int target_file = square % BoardGeometry::files;

  for(rank = target_rank + 1; rank <= 7; ++rank) {
    attacks |= (one << (rank * BoardGeometry::files + target_file));
    if((one << (rank * BoardGeometry::files + target_file)) & block)
      break;
  }

  for(rank = target_rank - 1; rank >= 0; --rank) {
    attacks |= (one << (rank * BoardGeometry::files + target_file));
    if((one << (rank * BoardGeometry::files + target_file)) & block)
      break;
  }

  for(file = target_file + 1; file <= 7; ++file) {
    attacks |= (one << (target_rank * BoardGeometry::files + file));
    if((one << (target_rank * BoardGeometry::files + file)) & block)
      break;
  }

  for(file = target_file - 1; file >= 0; --file) {
    attacks |= (one << (target_rank * BoardGeometry::files + file));
    if((one << (target_rank * BoardGeometry::files + file)) & block)
      break;
  }

  return attacks;
}

void init_leapers_attacks() {
  for(Squares square{ a8 }; square < no_square; ++square) {
    AttackTables::pawn[white][square] = mask_pawn_attacks(white, square);
    AttackTables::pawn[black][square] = mask_pawn_attacks(black, square);
    AttackTables::knight[square] = mask_knight_attacks(square);
    AttackTables::king[square] = mask_king_attacks(square);
  }
}

u64 set_occupancy(const int index, const int bits_in_mask, u64 attack_mask) {
  u64 occupancy{ };

  for(int count{ }; count < bits_in_mask; count++) {
    int square = get_ls1b_index(attack_mask);
    pop_bit(attack_mask, to_square(square));

    if(index & (one << count))
      occupancy |= (one << square);
  }

  return occupancy;
}

void init_sliders_attacks(const Sliders bishop) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const u64 attack_mask = bishop == Sliders::bishop ? AttackTables::bishop_masks[square] = mask_bishop_attacks(square)
                                                      : AttackTables::rook_masks[square] = mask_rook_attacks(square);
    const int relevant_bits_count = count_bits(attack_mask);
    const int occupancy_indices = (1 << relevant_bits_count);

    for(int index{ }; index < occupancy_indices; index++) {
      if(bishop == Sliders::bishop) {
        const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        const u64 magic_index = (occupancy * MagicTables::bishop_numbers[square]) >> (BoardGeometry::squares - MagicTables::bishop_relevant_bits[square]);
        AttackTables::bishop[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
      } else {
        const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        const u64 magic_index = (occupancy * MagicTables::rook_numbers[square]) >> (BoardGeometry::squares - MagicTables::rook_relevant_bits[square]);
        AttackTables::rook[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
      }
    }
  }
}

void AttackTables::init() {
  init_leapers_attacks();
  init_sliders_attacks(Sliders::bishop);
  init_sliders_attacks(Sliders::rook);
}

} // namespace maharajah
