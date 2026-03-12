#include "../headers/Bitboard.h"
#include "../headers/tables/AtacksTable.h"
#include "../headers/tables/BishopFlyAttacksTable.h"
#include "../headers/tables/OccupancyBishopTable.h"
#include "../headers/tables/OccupancyRookTable.h"
#include "../headers/tables/RookFlyAttacksTable.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class bitboard_test_fixture : public testing::Test {
  protected:
  void SetUp() override;
};

/**
 * Initializes all bitboards and attack tables.
 */
void bitboard_test_fixture::SetUp() {
  init_all();
}

/**
 * Tests whether the bitboards are initialized correctly.
 */
TEST_F(bitboard_test_fixture, board_test) {
  EXPECT_EQ(board[white], 0xFFFF000000000000);
  EXPECT_EQ(board[black], 0xFFFF);
}

/**
 * Tests whether setting a bit in a bitboard works correctly.
 *
 * This test function initializes an empty bitboard and then sets each
 * bit in the bitboard one by one. It checks whether the bit has been set
 * correctly by comparing the bitboard with the expected value (one
 * shifted to the left by the square index). The bitboard is then reset
 * to zero before the next iteration.
 */
TEST_F(bitboard_test_fixture, set_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(bitboard, (one << square));
    bitboard = zero;
  }
}

/**
 * Tests whether the get_bit function works correctly.
 *
 * This test function initializes an empty bitboard and then sets each
 * bit in the bitboard one by one. It checks whether the bit has been retrieved
 * correctly by comparing the result of get_bit with true. The bitboard is then
 * reset to zero before the next iteration.
 */
TEST_F(bitboard_test_fixture, get_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(get_bit(bitboard, square), true);
    bitboard = zero;
  }
}

/**
 * Tests whether the get_bit function returns false when given an empty bitboard.
 *
 * This test function checks whether the get_bit function returns false for all
 * squares when given an empty bitboard. This is a sanity check to ensure that
 * get_bit returns false when there are no bits set in the bitboard.
 */
TEST_F(bitboard_test_fixture, get_bit_false_on_empty_board) {
  constexpr u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_FALSE(get_bit(bitboard, square));
  }
}

/**
 * Tests whether the pop_bit function works correctly.
 *
 * This test function initializes an empty bitboard and then sets each
 * bit in the bitboard one by one. It checks whether the bit has been
 * popped correctly by comparing the result of pop_bit with zero and the
 * bitboard with zero. The bitboard is then reset to zero before the next
 * iteration.
 */
TEST_F(bitboard_test_fixture, pop_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(pop_bit(bitboard, square), zero);
    EXPECT_EQ(bitboard, zero);
  }
}

/**
 * Tests whether the pop_bit function preserves other bits in the bitboard.
 *
 * This test function sets two bits in the bitboard (a8 and h1) and then pops one
 * of them (a8). It checks whether the other bit (h1) has been preserved by
 * comparing the bitboard and the returned value with the expected value.
 */
TEST_F(bitboard_test_fixture, pop_bit_preserves_other_bits) {
  u64 bitboard{ zero };
  set_bit(bitboard, a8);
  set_bit(bitboard, h1);

  constexpr u64 expected = (one << h1);
  const u64 returned = pop_bit(bitboard, a8);

  EXPECT_EQ(bitboard, expected);
  EXPECT_EQ(returned, expected);
}

/**
 * Tests whether the count_bits function works correctly.
 *
 * This test function checks whether the count_bits function returns the correct
 * number of bits set in a bitboard. It does this by setting each bit in the
 * bitboard one by one and then comparing the result of count_bits with the
 * square index. The bitboard is then reset to zero before the next iteration.
 */
TEST_F(bitboard_test_fixture, count_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(square, count_bits((one << square) - 1));
  }
}

/**
 * Tests the count_bits function on edge cases:
 * zero, ~zero, one, one shifted to the left by 63, and one shifted to the left by 63 or one.
 */
TEST_F(bitboard_test_fixture, count_bits_edge_cases) {
  EXPECT_EQ(count_bits(zero), 0);
  EXPECT_EQ(count_bits(~zero), 64);
  EXPECT_EQ(count_bits(one), 1);
  EXPECT_EQ(count_bits(one << 63), 1);
  EXPECT_EQ(count_bits((one << 63) | one), 2);
}

/**
 * Tests whether the get_ls1b_index function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * get_ls1b_index function returns the correct index. It does this by
 * comparing the result of get_ls1b_index with the square index when the
 * bitboard is set to one shifted to the left by the square index.
 */
TEST_F(bitboard_test_fixture, get_ls1b_index_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(square, get_ls1b_index(one << square));
  }
}
/**
 * Tests whether the get_ls1b_index function works correctly when the bitboard has multiple bits set.
 *
 * This test function checks whether the get_ls1b_index function returns the correct index when the bitboard has multiple bits set.
 * It does this by setting three bits in the bitboard (c3, a8, and h1) and then comparing the result of get_ls1b_index with the expected index (a8).
 */
TEST_F(bitboard_test_fixture, get_ls1b_index_multiple_bits) {
  constexpr u64 mask = (one << c3) | (one << a8) | (one << h1);
  EXPECT_EQ(get_ls1b_index(mask), a8);
}

/**
 * Tests whether the mask_pawn_attacks function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * mask_pawn_attacks function returns the correct attack mask for white and
 * black pawns. It does this by comparing the result of
 * mask_pawn_attacks with the correct attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, mask_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[white_pawn_index][square], mask_pawn_attacks(white, square));
    EXPECT_EQ(attacks_table[black_pawn_index][square], mask_pawn_attacks(black, square));
  }
}

/**
 * Tests whether the mask_knight_attacks function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * mask_knight_attacks function returns the correct attack mask for knights.
 * It does this by comparing the result of mask_knight_attacks with the correct
 * attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, mask_knight_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[knight_index][square], mask_knight_attacks(square));
  }
}
/**
 * Tests whether the mask_king_attacks function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * mask_king_attacks function returns the correct attack mask for kings.
 * It does this by comparing the result of mask_king_attacks with the correct
 * attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, mask_king_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[king_index][square], mask_king_attacks(square));
  }
}

/**
 * Tests whether the white_pawn_attacks table was correctly initialized.
 *
 * This test function iterates over all squares and checks whether the
 * white_pawn_attacks table contains the correct attack mask for white pawns.
 */
TEST_F(bitboard_test_fixture, white_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[white_pawn_index][square], pawn_attacks[white][square]);
  }
}

/**
 * Tests whether the black_pawn_attacks table was correctly initialized.
 *
 * This test function iterates over all squares and checks whether the
 * black_pawn_attacks table contains the correct attack mask for black pawns.
 */
TEST_F(bitboard_test_fixture, black_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[black_pawn_index][square], pawn_attacks[black][square]);
  }
}
/**
 * Tests whether the knight_attacks table was correctly initialized.
 *
 * This test function iterates over all squares and checks whether the
 * knight_attacks table contains the correct attack mask for knights.
 */
TEST_F(bitboard_test_fixture, knight_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[knight_index][square], knight_attacks[square]);
  }
}
/**
 * Tests whether the mask_bishop_attacks function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * mask_bishop_attacks function returns the correct attack mask for bishops.
 * It does this by comparing the result of mask_bishop_attacks with the correct
 * attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, mask_bishop_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[bishop_index][square], mask_bishop_attacks(square));
  }
}
/**
 * Tests whether the mask_rook_attacks function works correctly.
 *
 * This test function iterates over all squares and checks whether the
 * mask_rook_attacks function returns the correct attack mask for rooks.
 * It does this by comparing the result of mask_rook_attacks with the correct
 * attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, mask_rook_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[rook_index][square], mask_rook_attacks(square));
  }
}
/**
 * Tests whether the king_attacks array is correctly generated for all squares.
 *
 * This test function iterates over all squares and checks whether the
 * king_attacks array contains the correct attack mask for kings on
 * that square. It does this by comparing the result of king_attacks with
 * the correct attack mask in the attacks_table.
 */
TEST_F(bitboard_test_fixture, king_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(attacks_table[king_index][square], king_attacks[square]);
  }
}
/**
 * Tests whether the bishop fly attacks table is correctly generated for a given square.
 *
 * This test function iterates over all squares and checks whether the
 * bishop_attacks_on_the_fly function returns the correct attack mask for bishops
 * on that square. It does this by comparing the result of
 * bishop_attacks_on_the_fly with the correct attack mask in the
 * bishop_fly_attacks_table.
 */
TEST_F(bitboard_test_fixture, bishop_fly_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << bishop_relevant_bits[square] };
    const u64 attack_mask{ mask_bishop_attacks(square) };
    for(int index{}; index < count; index++) {
      const u64 occupancies = set_occupancy(index, bishop_relevant_bits[square], attack_mask);
      EXPECT_EQ(bishop_fly_attacks_table[square][index], bishop_attacks_on_the_fly(square, occupancies));
    }
  }
}

/**
 * @brief Tests whether the rook fly attacks table is correctly generated for a given square.
 */
TEST_F(bitboard_test_fixture, rook_fly_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << rook_relevant_bits[square] };
    const u64 attack_mask{ mask_rook_attacks(square) };
    for(int index{}; index < count; index++) {
      const u64 occupancies{ set_occupancy(index, rook_relevant_bits[square], attack_mask) };
      EXPECT_EQ(rook_fly_attacks_table[square][index], rook_attacks_on_the_fly(square, occupancies));
    }
  }
}

/**
 * @brief Tests whether the square_to_coordinates function correctly maps a square to its coordinates.
 *
 * The test loops over all squares on the board and checks whether the square_to_coordinates function
 * correctly maps the square to its coordinates. The coordinates are compared with the precomputed
 * table, square_to_coord.
 */
TEST_F(bitboard_test_fixture, square_to_coordinates_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(square_to_coord[square], square_to_coord[get_ls1b_index(one << square)]);
  }
}
/**
 * @brief Tests whether the set_occupancy function produces the correct occupancy table for a rook on a given square.
 *
 * The test loops over all squares on the board and checks whether the set_occupancy function produces
 * the correct occupancy table for a rook on that square.
 *
 * The occupancy table is compared with the precomputed table, occupancy_rook_table.
 */
TEST_F(bitboard_test_fixture, set_occupancy_rook_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << rook_relevant_bits[square] };
    for(int index{}; index < count; index++) {
      const u64 attack_mask{ mask_rook_attacks(square) };
      EXPECT_EQ(occupancy_rook_table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
    }
  }
}

/**
 * Tests whether the set_occupancy function produces the correct occupancy table for a bishop on a given square.
 *
 * The test loops over all squares on the board and checks whether the set_occupancy function produces 
 * the correct occupancy table for a bishop on that square.
 *
 * The occupancy table is compared with the precomputed table, occupancy_bishop_table.
 */
TEST_F(bitboard_test_fixture, set_occupancy_bishop_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << bishop_relevant_bits[square] };
    for(int index{}; index < count; index++) {
      const u64 attack_mask{ mask_bishop_attacks(square) };
      EXPECT_EQ(occupancy_bishop_table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
    }
  }
}

/**
 * @brief Tests whether the set_occupancy function produces an occupancy table that is a subset of the attack mask.
 *
 * The test loops over all squares on the board and checks whether the set_occupancy function produces an occupancy table that is a subset of the attack mask.
 * It does this by checking whether the bitwise AND of the occupancy table and the bitwise complement of the attack mask is zero.
 */
TEST_F(bitboard_test_fixture, set_occupancy_is_subset_of_mask) {
  for(constexpr array<Squares, 3> sample{ a8, d4, h1 }; const auto square : sample) {
    const u64 attack_mask = mask_rook_attacks(square);
    const int count = 1 << rook_relevant_bits[square];
    for(int index{}; index < count; ++index) {
      const u64 occupancy = set_occupancy(index, count_bits(attack_mask), attack_mask);
      EXPECT_EQ(occupancy & ~attack_mask, zero);
    }
  }
}

/**
 * @brief Tests whether the number of relevant bits for each square is correctly initialized.
 *
 * This test iterates over all squares and checks whether the number of relevant bits for
 * that given square is correctly initialized in the bishop_relevant_bits table.
 */
TEST_F(bitboard_test_fixture, bishop_relevant_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(bishop_relevant_bits[square], count_bits(mask_bishop_attacks(square)));
  }
}
/**
 * @brief Tests whether the number of relevant bits for each square is correctly initialized.
 *
 * This test iterates over all squares and checks whether the number of relevant bits for 
 * that given square is correctly initialized in the rook_relevant_bits table.
 */
TEST_F(bitboard_test_fixture, rook_relevant_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(rook_relevant_bits[square], count_bits(mask_rook_attacks(square)));
  }
}
/**
 * @brief Tests whether the rook attacks for each square and occupancy are correctly initialized.
 *
 * This test iterates over all squares and occupancies and checks whether the rook attacks for that 
 * given square and occupancy are correctly initialized in the rook_attacks table.
 */
TEST_F(bitboard_test_fixture, init_rook_sliders_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << rook_relevant_bits[square] };

    for(int index{}; index < occupancy_indices; index++) {
      const u64 occupancy{ occupancy_rook_table[square][index] };
      const u64 magic_index{ (occupancy * rook_magic_numbers_table[square]) >> (end_bit - rook_relevant_bits[square]) };
      EXPECT_EQ(rook_attacks[square][magic_index], rook_attacks_on_the_fly(square, occupancy));
    }
  }
}

/**
 * @brief Tests whether the bishop attacks for each square and occupancy are correctly initialized.
 *
 * This test iterates over all squares and occupancies and checks whether the bishop attacks for that given square and occupancy are correctly
 * initialized. It does this by first getting the occupancy and magic index for the given square and occupancy, and then asserting that the bishop
 * attacks for that given square and occupancy are equal to the bishop attacks on the fly for that given square and occupancy.
 */
TEST_F(bitboard_test_fixture, init_bishop_sliders_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << bishop_relevant_bits[square] };

    for(int index{}; index < occupancy_indices; index++) {
      const u64 occupancy{ occupancy_bishop_table[square][index] };
      const u64 magic_index{ (occupancy * bishop_magic_numbers_table[square]) >> (end_bit - bishop_relevant_bits[square]) };
      EXPECT_EQ(bishop_attacks[square][magic_index], bishop_attacks_on_the_fly(square, occupancy));
    }
  }
}

/**
 * @brief Tests whether the get_rook_attacks function returns the correct rook attacks for all squares on the board.
 *
 * The get_rook_attacks function takes a square and occupancy as input and returns the rook attacks for that given square and occupancy.
 * This test iterates over all squares on the board and checks whether the get_rook_attacks function returns the 
 * correct rook attacks for all occupancies of the board.
 */
TEST_F(bitboard_test_fixture, get_rook_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << rook_relevant_bits[square] };

    for(int index{}; index < occupancy_indices; index++) {
      const u64 occupancy{ occupancy_rook_table[square][index] };
      const u64 magic_index{ (occupancy * rook_magic_numbers_table[square]) >> (end_bit - rook_relevant_bits[square]) };
      EXPECT_EQ(rook_attacks[square][magic_index], get_rook_attacks(square, occupancy));
    }
  }
}

/**
 * @brief Tests whether the get_bishop_attacks function returns the correct bishop attacks for all squares on the board.
 *
 * The get_bishop_attacks function takes a square and occupancy as input and returns the bishop attacks for that given square and occupancy.
 * It does this by first applying the bishop mask to the occupancy, then multiplying the occupancy by the bishop magic number,
 * and finally shifting the occupancy to the right by the number of relevant bits for the bishop.
 */
TEST_F(bitboard_test_fixture, get_bishop_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << bishop_relevant_bits[square] };

    for(int index{}; index < occupancy_indices; index++) {
      const u64 occupancy{ occupancy_bishop_table[square][index] };
      const u64 magic_index{ (occupancy * bishop_magic_numbers_table[square]) >> (end_bit - bishop_relevant_bits[square]) };
      EXPECT_EQ(bishop_attacks[square][magic_index], get_bishop_attacks(square, occupancy));
    }
  }
}

/**
 * @brief Tests whether the get_queen_attacks function returns the correct queen attacks for all squares on the board.
 *
 * The get_queen_attacks function takes a square and occupancy as input and returns the queen attacks for that given square and occupancy.
 * The function is tested by iterating over all squares on the board, and checking whether the result of the function call matches the result
 * of the bitwise OR operation of the get_bishop_attacks and get_rook_attacks functions for the given square and occupancy.
 */
TEST_F(bitboard_test_fixture, get_queen_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(get_bishop_attacks(square, zero) | get_rook_attacks(square, zero), get_queen_attacks(square, zero));
  }
}

/**
 * @brief Tests whether the get_queen_attacks function returns the correct queen attacks for a given square and non-zero occupancy.
 *
 * The get_queen_attacks function takes a square and occupancy as input and returns the queen attacks for that given square and occupancy.
 * The function is tested by setting the occupancy of the board to a non-zero value, and then checking whether the result of the function call matches the result
 * of the bitwise OR operation of the get_bishop_attacks and get_rook_attacks functions for the given square and occupancy.
 */
TEST_F(bitboard_test_fixture, get_queen_attacks_nonzero_occupancy) {
  constexpr Squares square = d4;
  u64 occupancy{ zero };
  set_bit(occupancy, d6);
  set_bit(occupancy, f4);
  set_bit(occupancy, b2);

  const u64 expected = get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
  EXPECT_EQ(expected, get_queen_attacks(square, occupancy));
}
