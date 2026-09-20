#include "../headers/BoardState.h"
#include "gtest/gtest.h"

#include <array>

using namespace std;
using namespace maharajah;

class board_state_test_fixture : public testing::Test {
  protected:
  void SetUp() override;

  public:
  BoardState board_state{ };
};

void board_state_test_fixture::SetUp() { }

TEST_F(board_state_test_fixture, test_initial_board_state) {
  EXPECT_EQ(board_state.side, white);
  EXPECT_EQ(board_state.en_passant, no_square);
  EXPECT_EQ(board_state.castle, 0);
  EXPECT_EQ(board_state.bitboards.size(), 13);
  EXPECT_EQ(board_state.occupancies.size(), 3);

  for(const auto& bitboard : board_state.bitboards) {
    EXPECT_EQ(bitboard, zero);
  }

  for(const auto& occupancy : board_state.occupancies) {
    EXPECT_EQ(occupancy, zero);
  }
}

TEST_F(board_state_test_fixture, test_init) {
  array<u64, 13> bitboards{
    0x000000000000FF00ULL, 0x00000000000000FFULL, zero, zero, zero, zero, 0x00FF000000000000ULL, zero, 0x0000000000018000ULL, 0x18000000000000ULL,
    0x80010000000000ULL,   0x01008000000000ULL,   zero
  };

  array<u64, 3> occupancies{ 0xFFFFULL, 0xFFFFULL, 0x1FFFFULL };
  board_state = BoardState(black, e3, wk | bq, bitboards, occupancies);

  EXPECT_EQ(board_state.side, black);
  EXPECT_EQ(board_state.en_passant, e3);
  EXPECT_EQ(board_state.castle, wk | bq);
  EXPECT_EQ(board_state.bitboards, bitboards);
  EXPECT_EQ(board_state.occupancies, occupancies);
}

TEST_F(board_state_test_fixture, default_constructed_state_is_independent) {
  BoardState other{ };
  other.side = black;
  other.en_passant = e3;
  other.castle = wk | bq;
  other.bitboards[0] = 0x1234ULL;
  other.occupancies[0] = 0x5678ULL;

  EXPECT_EQ(board_state.side, white);
  EXPECT_EQ(board_state.en_passant, no_square);
  EXPECT_EQ(board_state.castle, 0);
  EXPECT_EQ(board_state.bitboards[0], zero);
  EXPECT_EQ(board_state.occupancies[0], zero);
}
