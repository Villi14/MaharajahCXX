#include "../headers/Bitboard.h"
#include "../headers/tables/MagicNumbersTable.h"
#include "gtest/gtest.h"

#include <vector>

using namespace std;
using namespace maharajah;

class magic_numbers_test_fixture : public testing::Test {
  protected:
  void SetUp() override;
};

void magic_numbers_test_fixture::SetUp() {
  AttackTables::init();
}

bool test_magic_square(Squares square, u64 magic, int relevant_bits, bool bishop) {
  const int shift{ 64 - relevant_bits };
  const int occ_count{ 1 << relevant_bits };

  vector<u64> used(occ_count, 0ULL);
  vector<bool> filled(occ_count, false);

  for(int i{ }; i < occ_count; ++i) {
    u64 occ = set_occupancy(i, relevant_bits, bishop ? AttackTables::bishop_masks[square] : AttackTables::rook_masks[square]);

    u64 attack = bishop ? bishop_attacks_on_the_fly(square, occ) : rook_attacks_on_the_fly(square, occ);

    if(const u64 index = (occ * magic) >> shift; !filled[index]) {
      filled[index] = true;
      used[index] = attack;
    } else if(used[index] != attack) {
      return false; // collision
    }
  }

  return true;
}

TEST_F(magic_numbers_test_fixture, magic_numbers_nonzero) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    EXPECT_NE(MagicTables::bishop_numbers[square], 0ULL);
    EXPECT_NE(MagicTables::rook_numbers[square], 0ULL);
  }
}

TEST_F(magic_numbers_test_fixture, bishop_magics_are_collision_free) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    ASSERT_TRUE(test_magic_square(square, MagicTables::bishop_numbers[square], MagicTables::bishop_relevant_bits[square], true))
        << "Bishop magic collision at square " << square;
  }
}

TEST_F(magic_numbers_test_fixture, rook_magics_are_collision_free) {
  for(Squares square{ a8 }; square < no_square; ++square) {
    ASSERT_TRUE(test_magic_square(square, MagicTables::rook_numbers[square], MagicTables::rook_relevant_bits[square], false))
        << "Rook magic collision at square " << square;
  }
}
