#include "../headers/Bitboard.h"
#include "../headers/Notation.h"
#include "../headers/tables/AtacksTable.h"
#include "../headers/tables/BishopFlyAttacksTable.h"
#include "../headers/tables/OccupancyBishopTable.h"
#include "../headers/tables/OccupancyRookTable.h"
#include "../headers/tables/RookFlyAttacksTable.h"
#include "TestPositions.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class bitboard_test_fixture : public testing::Test {
  protected:
  void SetUp() override;
};

void bitboard_test_fixture::SetUp() {
  AttackTables::init();
}

TEST_F(bitboard_test_fixture, board_test) {
  EXPECT_EQ(StartBitboards::occupancy[white], 0xFFFF000000000000);
  EXPECT_EQ(StartBitboards::occupancy[black], 0xFFFF);
}

TEST_F(bitboard_test_fixture, set_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(bitboard, (one << square));
    bitboard = zero;
  }
}

TEST_F(bitboard_test_fixture, get_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(get_bit(bitboard, square), true);
    bitboard = zero;
  }
}

TEST_F(bitboard_test_fixture, get_bit_false_on_empty_board) {
  constexpr u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_FALSE(get_bit(bitboard, square));
  }
}

TEST_F(bitboard_test_fixture, pop_bit_test) {
  u64 bitboard{ zero };
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_bit(bitboard, square);
    EXPECT_EQ(pop_bit(bitboard, square), zero);
    EXPECT_EQ(bitboard, zero);
  }
}

TEST_F(bitboard_test_fixture, pop_bit_preserves_other_bits) {
  u64 bitboard{ zero };
  set_bit(bitboard, a8);
  set_bit(bitboard, h1);

  constexpr u64 expected = (one << h1);
  const u64 returned = pop_bit(bitboard, a8);

  EXPECT_EQ(bitboard, expected);
  EXPECT_EQ(returned, expected);
}

TEST_F(bitboard_test_fixture, count_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(square, count_bits((one << square) - 1));
  }
}

TEST_F(bitboard_test_fixture, count_bits_edge_cases) {
  EXPECT_EQ(count_bits(zero), 0);
  EXPECT_EQ(count_bits(~zero), 64);
  EXPECT_EQ(count_bits(one), 1);
  EXPECT_EQ(count_bits(one << 63), 1);
  EXPECT_EQ(count_bits((one << 63) | one), 2);
}

TEST_F(bitboard_test_fixture, get_ls1b_index_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(square, get_ls1b_index(one << square));
  }
}
TEST_F(bitboard_test_fixture, get_ls1b_index_multiple_bits) {
  constexpr u64 mask = (one << c3) | (one << a8) | (one << h1);
  EXPECT_EQ(get_ls1b_index(mask), a8);
}

TEST_F(bitboard_test_fixture, mask_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::white_pawn][square], mask_pawn_attacks(white, square));
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::black_pawn][square], mask_pawn_attacks(black, square));
  }
}

TEST_F(bitboard_test_fixture, mask_knight_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::knight][square], mask_knight_attacks(square));
  }
}
TEST_F(bitboard_test_fixture, mask_king_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::king][square], mask_king_attacks(square));
  }
}

TEST_F(bitboard_test_fixture, white_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::white_pawn][square], AttackTables::pawn[white][square]);
  }
}

TEST_F(bitboard_test_fixture, black_pawn_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::black_pawn][square], AttackTables::pawn[black][square]);
  }
}
TEST_F(bitboard_test_fixture, knight_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::knight][square], AttackTables::knight[square]);
  }
}
TEST_F(bitboard_test_fixture, mask_bishop_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::bishop][square], mask_bishop_attacks(square));
  }
}
TEST_F(bitboard_test_fixture, mask_rook_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::rook][square], mask_rook_attacks(square));
  }
}
TEST_F(bitboard_test_fixture, king_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(LeaperAttacks::table[LeaperAttacks::king][square], AttackTables::king[square]);
  }
}
TEST_F(bitboard_test_fixture, bishop_fly_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << MagicTables::bishop_relevant_bits[square] };
    const u64 attack_mask{ mask_bishop_attacks(square) };
    for(int index{ }; index < count; ++index) {
      const u64 occupancies = set_occupancy(index, MagicTables::bishop_relevant_bits[square], attack_mask);
      EXPECT_EQ(BishopFlyAttacks::table[square][index], bishop_attacks_on_the_fly(square, occupancies));
    }
  }
}

TEST_F(bitboard_test_fixture, rook_fly_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << MagicTables::rook_relevant_bits[square] };
    const u64 attack_mask{ mask_rook_attacks(square) };
    for(int index{ }; index < count; ++index) {
      const u64 occupancies{ set_occupancy(index, MagicTables::rook_relevant_bits[square], attack_mask) };
      EXPECT_EQ(RookFlyAttacks::table[square][index], rook_attacks_on_the_fly(square, occupancies));
    }
  }
}

TEST_F(bitboard_test_fixture, square_to_coordinates_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(Notation::square_to_coordinates[square], Notation::square_to_coordinates[get_ls1b_index(one << square)]);
  }
}
TEST_F(bitboard_test_fixture, set_occupancy_rook_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << MagicTables::rook_relevant_bits[square] };
    for(int index{ }; index < count; ++index) {
      const u64 attack_mask{ mask_rook_attacks(square) };
      EXPECT_EQ(RookOccupancy::table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
    }
  }
}

TEST_F(bitboard_test_fixture, set_occupancy_bishop_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int count{ 1 << MagicTables::bishop_relevant_bits[square] };
    for(int index{ }; index < count; ++index) {
      const u64 attack_mask{ mask_bishop_attacks(square) };
      EXPECT_EQ(BishopOccupancy::table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
    }
  }
}

TEST_F(bitboard_test_fixture, set_occupancy_is_subset_of_mask) {
  for(constexpr array<Squares, 3> sample{ a8, d4, h1 }; const auto square : sample) {
    const u64 attack_mask = mask_rook_attacks(square);
    const int count = 1 << MagicTables::rook_relevant_bits[square];
    for(int index{ }; index < count; ++index) {
      const u64 occupancy = set_occupancy(index, count_bits(attack_mask), attack_mask);
      EXPECT_EQ(occupancy & ~attack_mask, zero);
    }
  }
}

TEST_F(bitboard_test_fixture, bishop_relevant_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(MagicTables::bishop_relevant_bits[square], count_bits(mask_bishop_attacks(square)));
  }
}
TEST_F(bitboard_test_fixture, rook_relevant_bits_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(MagicTables::rook_relevant_bits[square], count_bits(mask_rook_attacks(square)));
  }
}
TEST_F(bitboard_test_fixture, init_rook_sliders_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << MagicTables::rook_relevant_bits[square] };

    for(int index{ }; index < occupancy_indices; ++index) {
      const u64 occupancy{ RookOccupancy::table[square][index] };
      const u64 magic_index{ (occupancy * MagicTables::rook_numbers[square]) >> (BoardGeometry::squares - MagicTables::rook_relevant_bits[square]) };
      EXPECT_EQ(AttackTables::rook[square][magic_index], rook_attacks_on_the_fly(square, occupancy));
    }
  }
}

TEST_F(bitboard_test_fixture, init_bishop_sliders_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << MagicTables::bishop_relevant_bits[square] };

    for(int index{ }; index < occupancy_indices; ++index) {
      const u64 occupancy{ BishopOccupancy::table[square][index] };
      const u64 magic_index{ (occupancy * MagicTables::bishop_numbers[square]) >> (BoardGeometry::squares - MagicTables::bishop_relevant_bits[square]) };
      EXPECT_EQ(AttackTables::bishop[square][magic_index], bishop_attacks_on_the_fly(square, occupancy));
    }
  }
}

TEST_F(bitboard_test_fixture, get_rook_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << MagicTables::rook_relevant_bits[square] };

    for(int index{ }; index < occupancy_indices; index++) {
      const u64 occupancy{ RookOccupancy::table[square][index] };
      const u64 magic_index{ (occupancy * MagicTables::rook_numbers[square]) >> (BoardGeometry::squares - MagicTables::rook_relevant_bits[square]) };
      EXPECT_EQ(AttackTables::rook[square][magic_index], get_rook_attacks(square, occupancy));
    }
  }
}

TEST_F(bitboard_test_fixture, get_bishop_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    const int occupancy_indices{ 1 << MagicTables::bishop_relevant_bits[square] };

    for(int index{ }; index < occupancy_indices; ++index) {
      const u64 occupancy{ BishopOccupancy::table[square][index] };
      const u64 magic_index{ (occupancy * MagicTables::bishop_numbers[square]) >> (BoardGeometry::squares - MagicTables::bishop_relevant_bits[square]) };
      EXPECT_EQ(AttackTables::bishop[square][magic_index], get_bishop_attacks(square, occupancy));
    }
  }
}

TEST_F(bitboard_test_fixture, get_queen_attacks_test) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_EQ(get_bishop_attacks(square, zero) | get_rook_attacks(square, zero), get_queen_attacks(square, zero));
  }
}

TEST_F(bitboard_test_fixture, get_queen_attacks_nonzero_occupancy) {
  constexpr Squares square = d4;
  u64 occupancy{ zero };
  set_bit(occupancy, d6);
  set_bit(occupancy, f4);
  set_bit(occupancy, b2);

  const u64 expected = get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
  EXPECT_EQ(expected, get_queen_attacks(square, occupancy));
}
