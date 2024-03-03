#include <gtest/gtest.h>
#include <vector>
#include <Bitboard.h>
#include "AtacksTable.h"
#include "BishopFlyAttacks.h"
#include "OccupancyBishopTable.h"
#include "OccupancyRookTable.h"
#include "RookFlyAttacks.h"

using namespace std;
using namespace m;

class bitboard_test_fixture : public testing::Test {
public:
	bitboard_test_fixture() { init_all(); }
};

TEST_F(bitboard_test_fixture, board) {
	EXPECT_EQ(board[0], 0xFFFF000000000000);
	EXPECT_EQ(board[1], 0xFFFF);
}

TEST_F(bitboard_test_fixture, white_pawn_attacks) {
	for (const auto square : all_squares)
		EXPECT_EQ(attacks_table[white_pawn_index][square], pawn_attacks[white][square]);
}

TEST_F(bitboard_test_fixture, black_pawn_attacks) {
	for (const auto square : all_squares)
		EXPECT_EQ(attacks_table[black_pawn_index][square], pawn_attacks[black][square]);
}

TEST_F(bitboard_test_fixture, knight_attacks) {
	for (const auto square : all_squares)
		EXPECT_EQ(attacks_table[knight_index][square], knight_attacks[square]);
}

TEST_F(bitboard_test_fixture, king_attacks) {
	for (const auto square : all_squares)
		EXPECT_EQ(attacks_table[king_index][square], king_attacks[square]);
}

TEST_F(bitboard_test_fixture, bishop_fly_attacks) {
	for (const auto square : all_squares) {
		const int count = 1 << bishop_relevant_bits[square];
		const u64 attack_mask = mask_bishop_attacks(square);
		for (int index{}; index < count; index++) {
			const u64 occupancies = set_occupancy(index, bishop_relevant_bits[square], attack_mask);
			EXPECT_EQ(bishop_fly_attacks_table[square][index], bishop_attacks_on_the_fly(square, occupancies));
		}
	}
}

TEST_F(bitboard_test_fixture, rook_fly_attacks) {
	for (const auto square : all_squares) {
		const int count = 1 << rook_relevant_bits[square];
		const u64 attack_mask = mask_rook_attacks(square);
		for (int index{}; index < count; index++) {
			const u64 occupancies = set_occupancy(index, rook_relevant_bits[square], attack_mask);
			EXPECT_EQ(rook_fly_attacks_table[square][index],
				rook_attacks_on_the_fly(static_cast<squares>(square), occupancies));
		}
	}
}

TEST_F(bitboard_test_fixture, count_bits) {
	for (const auto square : all_squares)
		EXPECT_EQ(square, count_bits((1ULL << square) - 1));
}

TEST_F(bitboard_test_fixture, get_ls1b_index) {
	for (const auto square : all_squares)
		EXPECT_EQ(square, get_ls1b_index(1ULL << square));
}

TEST_F(bitboard_test_fixture, square_to_coordinates) {
	for (const auto square : all_squares)
		EXPECT_EQ(square_to_coordinates[square], square_to_coordinates[get_ls1b_index(1ULL << square)]);
}

TEST_F(bitboard_test_fixture, set_occupancy_rook) {
	for (const auto square : all_squares) {
		const int count = 1 << rook_relevant_bits[square];
		for (int index{}; index < count; index++) {
			const u64 attack_mask = mask_rook_attacks(square);
			EXPECT_EQ(occupancy_rook_table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
		}
	}
}

TEST_F(bitboard_test_fixture, set_occupancy_bishop) {
	for (const auto square : all_squares) {
		const int count = 1 << bishop_relevant_bits[square];
		for (int index{}; index < count; index++) {
			const u64 attack_mask = mask_bishop_attacks(square);
			EXPECT_EQ(occupancy_bishop_table[square][index], set_occupancy(index, count_bits(attack_mask), attack_mask));
		}
	}
}

TEST_F(bitboard_test_fixture, bishop_relevant_bits) {
	for (const auto square : all_squares)
		EXPECT_EQ(bishop_relevant_bits[square], count_bits(mask_bishop_attacks(square)));
}

TEST_F(bitboard_test_fixture, rook_relevant_bits) {
	for (const auto square : all_squares)
		EXPECT_EQ(rook_relevant_bits[square], count_bits(mask_rook_attacks(square)));
}

TEST_F(bitboard_test_fixture, init_rook_sliders_attacks) {
	for (const squares square : all_squares) {
		const int occupancy_indices = (1 << rook_relevant_bits[square]);

		for (int index{}; index < occupancy_indices; index++) {
			const u64 occupancy = occupancy_rook_table[square][index];
			const u64 magic_index = (occupancy * rook_magic_numbers[square]) >> (end_bit - rook_relevant_bits[square]);
			EXPECT_EQ(rook_attacks[square][magic_index], rook_attacks_on_the_fly(square, occupancy));
		}
	}
}

TEST_F(bitboard_test_fixture, init_bishop_sliders_attacks) {
	for (const squares square : all_squares) {
		const int occupancy_indices = (1 << bishop_relevant_bits[square]);

		for (int index{}; index < occupancy_indices; index++) {
			const u64 occupancy = occupancy_bishop_table[square][index];
			const u64 magic_index = (occupancy * bishop_magic_numbers[square]) >> (end_bit - bishop_relevant_bits[square]);
			EXPECT_EQ(bishop_attacks[square][magic_index], bishop_attacks_on_the_fly(square, occupancy));
		}
	}
}

TEST_F(bitboard_test_fixture, get_rook_attacks) {
	for (const squares square : all_squares) {
		const int occupancy_indices = (1 << rook_relevant_bits[square]);

		for (int index{}; index < occupancy_indices; index++) {
			const u64 occupancy = occupancy_rook_table[square][index];
			const u64 magic_index = (occupancy * rook_magic_numbers[square]) >> (end_bit - rook_relevant_bits[square]);
			EXPECT_EQ(rook_attacks[square][magic_index], get_rook_attacks(square, occupancy));
		}
	}
}

TEST_F(bitboard_test_fixture, get_bishop_attacks) {
	for (const squares square : all_squares) {
		const int occupancy_indices = (1 << bishop_relevant_bits[square]);

		for (int index{}; index < occupancy_indices; index++) {
			const u64 occupancy = occupancy_bishop_table[square][index];
			const u64 magic_index = (occupancy * bishop_magic_numbers[square]) >> (end_bit - bishop_relevant_bits[square]);
			EXPECT_EQ(bishop_attacks[square][magic_index], get_bishop_attacks(square, occupancy));
		}
	}
}