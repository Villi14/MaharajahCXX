#include "../headers/BoardState.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

TEST(board_state_test, default_state_is_an_empty_standard_board) {
  const BoardState state{ };

  EXPECT_EQ(state.side, white);
  EXPECT_EQ(state.en_passant, no_square);
  EXPECT_EQ(state.castle, 0);
  EXPECT_EQ(state.halfmove, 0);
  EXPECT_TRUE(state.standard_rules);
  EXPECT_FALSE(state.side_variant[white]);
  EXPECT_FALSE(state.side_variant[black]);
  EXPECT_EQ(state.pawn_unmoved, zero);
  EXPECT_EQ(state.hash_key, zero);
  for(const u64 bitboard : state.bitboards)
    EXPECT_EQ(bitboard, zero);
  for(const u64 occupancy : state.occupancies)
    EXPECT_EQ(occupancy, zero);
  EXPECT_FALSE(state.has_compound_pieces());
}

TEST(board_state_test, same_position_ignores_the_clocks_but_not_the_rights) {
  BoardState state{ };
  state.bitboards[K] = one << e1;
  state.bitboards[k] = one << e8;

  BoardState other = state;
  other.halfmove = 40;
  other.hash_key = 0x1234;
  EXPECT_TRUE(state.same_position(other));

  other.castle = wk;
  EXPECT_FALSE(state.same_position(other));
  other = state;
  other.en_passant = e3;
  EXPECT_FALSE(state.same_position(other));
  other = state;
  other.side = black;
  EXPECT_FALSE(state.same_position(other));
}
