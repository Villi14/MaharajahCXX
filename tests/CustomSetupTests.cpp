// Generated-army constraints, ported from MaharajahC's custom_setup_smoke.c.

#include "../headers/Bitboard.h"
#include "../headers/CustomSetup.h"
#include "../headers/Engine.h"
#include "gtest/gtest.h"

#include <array>
#include <utility>

using namespace std;
using namespace maharajah;

namespace {

// The budget and piece costs published in docs/ui/game_rules.md section 5. Kept
// here on purpose rather than read from the generator, so a silent edit of the
// generator's table fails this test.
constexpr int army_budget{ 39 };
constexpr array<pair<Pieces, int>, 8> white_piece_costs{ {
  { P, 1 }, { N, 3 }, { B, 3 }, { R, 5 }, { Q, 9 }, { A, 6 }, { C, 8 }, { M, 13 },
} };
constexpr int minimum_pawns{ 4 };

int count(const Board& board, const Pieces white_piece, const Colors side) {
  return count_bits(board.state.bitboards[white_piece + (side == white ? 0 : PieceCount::per_side)]);
}

int army_weight(const Board& board, const Colors side) {
  int weight{ };
  for(const auto& [piece, cost] : white_piece_costs)
    weight += count(board, piece, side) * cost;
  return weight;
}

} // namespace

// Armies used to be spent almost entirely on the home rank (0.9 pawns on average,
// 41% without any). Every army now needs a pawn wall and, since the position is
// variant on both sides, at least one compound piece.
TEST(custom_setup_test, generated_armies_spend_the_budget_with_a_pawn_wall_and_a_compound_piece) {
  Engine engine;
  Board& board = engine.board;
  for(unsigned int seed{ 1 }; seed <= 200U; ++seed) {
    ASSERT_TRUE(generate_custom_position(board, white, seed)) << "seed " << seed;

    for(const Colors side : { white, black }) {
      SCOPED_TRACE(testing::Message() << "seed " << seed << (side == white ? " white" : " black"));
      EXPECT_EQ(count(board, K, side), 1);
      EXPECT_EQ(army_weight(board, side), army_budget);
      EXPECT_GE(count(board, P, side), minimum_pawns);
      EXPECT_GE(count(board, A, side) + count(board, C, side) + count(board, M, side), 1);
    }
  }
}
