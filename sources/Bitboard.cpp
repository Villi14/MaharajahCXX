#include "../headers/Bitboard.h"
#include "../headers/tables/MagicNumbersTable.h"

using std::array;

namespace maharajah {

array<array<u64, end_bit>, 2> pawn_attacks{};
array<u64, end_bit> knight_attacks{};
array<u64, end_bit> king_attacks{};
array<u64, end_bit> bishop_masks{};
array<u64, end_bit> rook_masks{};
array<array<u64, bishop_attacks_count>, end_bit> bishop_attacks{};
array<array<u64, rook_attacks_count>, end_bit> rook_attacks{};

/**
 * @brief Generate a pawn attack mask for a given square and color
 *
 * @param color The color of the pawn
 * @param square The square to generate the attack mask for
 * @return The attack mask for the pawn
 */
u64 mask_pawn_attacks(const Colors color, const Squares square) {
  u64 attacks{};
  u64 bitboard{};

  set_bit(bitboard, square);

  if(color == white) {
    if((bitboard >> 7) & not_a_file)
      attacks |= (bitboard >> 7);
    if((bitboard >> 9) & not_h_file)
      attacks |= (bitboard >> 9);
  } else {
    if((bitboard << 7) & not_h_file)
      attacks |= (bitboard << 7);
    if((bitboard << 9) & not_a_file)
      attacks |= (bitboard << 9);
  }

  return attacks;
}

/**
 * @brief Generate a knight attack mask for a given square
 *
 * @param square The square to generate the attack mask for
 * @return The attack mask for the knight
 */
u64 mask_knight_attacks(const Squares square) {
  u64 attacks{};
  u64 bitboard{};

  set_bit(bitboard, square);

  if((bitboard >> 17) & not_h_file)
    attacks |= (bitboard >> 17);
  if((bitboard >> 15) & not_a_file)
    attacks |= (bitboard >> 15);
  if((bitboard >> 10) & not_gh_file)
    attacks |= (bitboard >> 10);
  if((bitboard >> 6) & not_ab_file)
    attacks |= (bitboard >> 6);
  if((bitboard << 17) & not_a_file)
    attacks |= (bitboard << 17);
  if((bitboard << 15) & not_h_file)
    attacks |= (bitboard << 15);
  if((bitboard << 10) & not_ab_file)
    attacks |= (bitboard << 10);
  if((bitboard << 6) & not_gh_file)
    attacks |= (bitboard << 6);

  return attacks;
}

/**
 * @brief Generate a king attack mask for a given square
 *
 * @param square The square to generate the attack mask for
 * @return The attack mask for the king
 *
 * The king can move in any direction (horizontally, vertically, or diagonally) one square.
 * The king is blocked by other pieces on the board and cannot move onto a square that is under attack by an opponent's piece.
 */
u64 mask_king_attacks(const Squares square) {
  u64 attacks{};
  u64 bitboard{};

  set_bit(bitboard, square);

  if(bitboard >> 8)
    attacks |= (bitboard >> 8);
  if((bitboard >> 9) & not_h_file)
    attacks |= (bitboard >> 9);
  if((bitboard >> 7) & not_a_file)
    attacks |= (bitboard >> 7);
  if((bitboard >> 1) & not_h_file)
    attacks |= (bitboard >> 1);
  if(bitboard << 8)
    attacks |= (bitboard << 8);
  if((bitboard << 9) & not_a_file)
    attacks |= (bitboard << 9);
  if((bitboard << 7) & not_h_file)
    attacks |= (bitboard << 7);
  if((bitboard << 1) & not_a_file)
    attacks |= (bitboard << 1);

  return attacks;
}

/**
 * @brief Generate a bishop attack mask for a given square
 *
 * @param square The square to generate the attack mask for
 * @return The attack mask for the bishop
 *
 * The bishop can move in any diagonal direction (northwest, northeast, southwest, southeast) any number of squares.
 * The bishop is blocked by other pieces on the board and cannot move onto a square that is under attack by an opponent's piece.
 */
u64 mask_bishop_attacks(const Squares square) {
  u64 attacks{};
  int rank{}, file{};
  const int target_rank = square / rank_bit;
  const int target_file = square % file_bit;

  for(rank = target_rank + 1, file = target_file + 1; rank <= 6 && file <= 6; rank++, file++)
    attacks |= (one << (rank * file_bit + file));
  for(rank = target_rank - 1, file = target_file + 1; rank >= 1 && file <= 6; rank--, file++)
    attacks |= (one << (rank * file_bit + file));
  for(rank = target_rank + 1, file = target_file - 1; rank <= 6 && file >= 1; rank++, file--)
    attacks |= (one << (rank * file_bit + file));
  for(rank = target_rank - 1, file = target_file - 1; rank >= 1 && file >= 1; rank--, file--)
    attacks |= (one << (rank * file_bit + file));

  return attacks;
}

/**
 * @brief Generate a rook attack mask for a given square
 *
 * @param square The square to generate the attack mask for
 * @return The attack mask for the rook
 *
 * The rook can move in any horizontal or vertical direction (north, south, east, west) any number of squares.
 * The rook is blocked by other pieces on the board and cannot move onto a square that is under attack by an opponent's piece.
 */
u64 mask_rook_attacks(const Squares square) {
  u64 attacks{};
  int rank{}, file{};
  const int target_rank = square / rank_bit;
  const int target_file = square % file_bit;

  for(rank = target_rank + 1; rank <= 6; rank++)
    attacks |= (one << (rank * file_bit + target_file));
  for(rank = target_rank - 1; rank >= 1; rank--)
    attacks |= (one << (rank * file_bit + target_file));
  for(file = target_file + 1; file <= 6; file++)
    attacks |= (one << (target_rank * file_bit + file));
  for(file = target_file - 1; file >= 1; file--)
    attacks |= (one << (target_rank * file_bit + file));

  return attacks;
}

/**
 * @brief Generate a bishop attack mask for a given square, taking into account the given block.
 *
 * @param square The square to generate the attack mask for
 * @param block The block to check against
 * @return The attack mask for the bishop
 *
 * This function generates a bishop attack mask for a given square, taking into account the given block.
 * The bishop can move in any diagonal direction (northwest, northeast, southwest, southeast) any number of squares.
 * The bishop is blocked by other pieces on the board and cannot move onto a square that is under attack by an opponent's piece.
 */
u64 bishop_attacks_on_the_fly(const Squares square, const u64 block) {
  u64 attacks{};
  int rank{}, file{};
  const int target_rank = square / rank_bit;
  const int target_file = square % file_bit;

  for(rank = target_rank + 1, file = target_file + 1; rank <= 7 && file <= 7; rank++, file++) {
    attacks |= (one << (rank * file_bit + file));
    if((one << (rank * file_bit + file)) & block)
      break;
  }

  for(rank = target_rank - 1, file = target_file + 1; rank >= 0 && file <= 7; rank--, file++) {
    attacks |= (one << (rank * file_bit + file));
    if((one << (rank * file_bit + file)) & block)
      break;
  }

  for(rank = target_rank + 1, file = target_file - 1; rank <= 7 && file >= 0; rank++, file--) {
    attacks |= (one << (rank * file_bit + file));
    if((one << (rank * file_bit + file)) & block)
      break;
  }

  for(rank = target_rank - 1, file = target_file - 1; rank >= 0 && file >= 0; rank--, file--) {
    attacks |= (one << (rank * file_bit + file));
    if((one << (rank * file_bit + file)) & block)
      break;
  }

  return attacks;
}

/**
 * @brief Generate a rook attack mask for a given square, taking into account the given block.
 * @param square The square to generate the attack mask for
 * @param block The block to check against
 * @return The attack mask for the rook
 *
 * This function generates a rook attack mask for a given square, taking into account the given block.
 * The rook can move in any horizontal or vertical direction (north, south, east, west) any number of squares.
 * The rook is blocked by other pieces on the board and cannot move onto a square that is under attack by an opponent's piece.
 */
u64 rook_attacks_on_the_fly(const Squares square, const u64 block) {
  u64 attacks{};
  int rank{}, file{};
  const int target_rank = square / rank_bit;
  const int target_file = square % file_bit;

  for(rank = target_rank + 1; rank <= 7; rank++) {
    attacks |= (one << (rank * file_bit + target_file));
    if((one << (rank * file_bit + target_file)) & block)
      break;
  }

  for(rank = target_rank - 1; rank >= 0; rank--) {
    attacks |= (one << (rank * file_bit + target_file));
    if((one << (rank * file_bit + target_file)) & block)
      break;
  }

  for(file = target_file + 1; file <= 7; file++) {
    attacks |= (one << (target_rank * file_bit + file));
    if((one << (target_rank * file_bit + file)) & block)
      break;
  }

  for(file = target_file - 1; file >= 0; file--) {
    attacks |= (one << (target_rank * file_bit + file));
    if((one << (target_rank * file_bit + file)) & block)
      break;
  }

  return attacks;
}

/**
 * Initializes the leaper attack tables (pawn, knight, king).
 * This function should be called once, at the beginning of the program.
 */
void init_leapers_attacks() {
  for(Squares square{ a8 }; square < no_square; ++square) {
    pawn_attacks[white][square] = mask_pawn_attacks(white, square);
    pawn_attacks[black][square] = mask_pawn_attacks(black, square);
    knight_attacks[square] = mask_knight_attacks(square);
    king_attacks[square] = mask_king_attacks(square);
  }
}

/**
 * @brief Sets a bit in a u64 bitboard if the corresponding bit in the index is set.
 *
 * @param index The index to check the bits of.
 * @param bits_in_mask The number of bits in the attack mask.
 * @param attack_mask The attack mask to set the bits in.
 * @return The updated bitboard with the bits set.
 */
u64 set_occupancy(const int index, const int bits_in_mask, u64 attack_mask) {
  u64 occupancy{};

  for(int count{}; count < bits_in_mask; count++) {
    int square = get_ls1b_index(attack_mask);
    pop_bit(attack_mask, to_square(square));

    if(index & (one << count))
      occupancy |= (one << square);
  }

  return occupancy;
}

/**
 * Initializes the magic number attack tables for the given slider (bishop or rook).
 * This function should be called once, at the beginning of the program.
 *
 * @param bishop The slider to initialize the attack tables for.
 */
void init_sliders_attacks(const Sliders bishop) {
 for(Squares square{ a8 }; square < no_square; ++square) {
    const u64 attack_mask = bishop == Sliders::bishop ? bishop_masks[square] = mask_bishop_attacks(square) : rook_masks[square] = mask_rook_attacks(square);
    const int relevant_bits_count = count_bits(attack_mask);
    const int occupancy_indices = (1 << relevant_bits_count);

    for(int index{}; index < occupancy_indices; index++) {
      if(bishop == Sliders::bishop) {
        const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        const u64 magic_index = (occupancy * bishop_magic_numbers_table[square]) >> (end_bit - bishop_relevant_bits[square]);
        bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
      } else {
        const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        const u64 magic_index = (occupancy * rook_magic_numbers_table[square]) >> (end_bit - rook_relevant_bits[square]);
        rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
      }
    }
  }
}

/**
 * Initializes all attack tables (leaper and slider attacks).
 * This function should be called once, at the beginning of the program.
 */
void init_all() {
  init_leapers_attacks();
  init_sliders_attacks(Sliders::bishop);
  init_sliders_attacks(Sliders::rook);
}

} // namespace maharajah
