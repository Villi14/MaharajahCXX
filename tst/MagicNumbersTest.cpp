#include <gtest/gtest.h>
#include <Bitboard.h>
#include <MagicNumbers.h>

using namespace m;

class magic_numbers_test_fixture : public testing::Test {
public:
  magic_numbers_test_fixture() { init_all(); }
};

TEST_F(magic_numbers_test_fixture, rook_find_magic_number) {
  for (const auto square : all_squares)
    EXPECT_EQ(rook_magic_numbers[square], find_magic_number(square, rook_relevant_bits[square], rook));
}

TEST_F(magic_numbers_test_fixture, bishop_find_magic_number) {
	for (const auto square : all_squares)
    EXPECT_EQ(bishop_magic_numbers[square], find_magic_number(square, bishop_relevant_bits[square], bishop));
}
