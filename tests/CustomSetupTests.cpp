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

namespace {

bool is_variant_army(const Board& board, const Colors side) {
  return board.state.side_variant[side];
}

// every piece of `side` stands on its own half (white ranks 1-4, black ranks 5-8)
bool army_on_own_half(const Board& board, const Colors side) {
  u64 own_half{ };
  for(int square{ }; square < 64; ++square) {
    const bool white_half = square / 8 >= 4;
    if(white_half == (side == white))
      set_bit(own_half, to_square(square));
  }
  return (board.state.occupancies[side] & ~own_half) == zero;
}

bool is_orthodox_army(const Board& board, const Colors side) {
  const array<pair<Pieces, int>, 6> orthodox{ { { P, 8 }, { N, 2 }, { B, 2 }, { R, 2 }, { Q, 1 }, { K, 1 } } };
  for(const auto& [piece, number] : orthodox) {
    if(count(board, piece, side) != number)
      return false;
  }
  return count(board, A, side) + count(board, C, side) + count(board, M, side) == 0;
}

} // namespace

// The players' generator draws one shape of army, so the training data had no pawn
// swarms, no knight armies, no armies without a compound piece and no orthodox army
// against a variant one. The training starts cover them, and stay valid 39-point armies.
TEST(custom_setup_test, wide_training_armies_cover_the_shapes_players_build) {
  Engine engine;
  Board& board = engine.board;
  int swarms{ }, nearly_pawnless{ }, without_compound{ }, knight_armies{ }, several_queens{ };
  constexpr unsigned int positions{ 400 };

  for(unsigned int seed{ 1 }; seed <= positions; ++seed) {
    ASSERT_TRUE(generate_training_position(board, TrainingStart::wide, seed)) << "seed " << seed;
    const string fen = board.to_fen();
    Board reparsed;
    ASSERT_TRUE(reparsed.parse_fen(fen)) << fen;
    EXPECT_EQ(reparsed.to_fen(), fen);

    for(const Colors side : { white, black }) {
      SCOPED_TRACE(testing::Message() << fen << (side == white ? " white" : " black"));
      EXPECT_TRUE(is_variant_army(board, side));
      EXPECT_EQ(count(board, K, side), 1);
      EXPECT_EQ(army_weight(board, side), army_budget);
      EXPECT_TRUE(army_on_own_half(board, side));

      const int pawns = count(board, P, side);
      swarms += pawns >= 20;
      nearly_pawnless += pawns <= 2;
      without_compound += count(board, A, side) + count(board, C, side) + count(board, M, side) == 0;
      knight_armies += count(board, N, side) >= 5;
      several_queens += count(board, Q, side) >= 2;
    }
  }

  // 800 armies; floors well under what seeds 1-400 give (knight armies: 26)
  EXPECT_GE(swarms, 80);
  EXPECT_GE(nearly_pawnless, 80);
  EXPECT_GE(without_compound, 80);
  EXPECT_GE(knight_armies, 15);
  EXPECT_GE(several_queens, 40);
}

TEST(custom_setup_test, mixed_training_starts_put_the_orthodox_army_against_a_variant_one) {
  Engine engine;
  Board& board = engine.board;
  int orthodox_white{ }, orthodox_black{ };
  constexpr unsigned int positions{ 200 };

  for(unsigned int seed{ 1 }; seed <= positions; ++seed) {
    ASSERT_TRUE(generate_training_position(board, TrainingStart::mixed, seed)) << "seed " << seed;
    const string fen = board.to_fen();
    SCOPED_TRACE(fen);
    Board reparsed;
    ASSERT_TRUE(reparsed.parse_fen(fen));
    EXPECT_EQ(reparsed.to_fen(), fen);

    const Colors orthodox = is_variant_army(board, white) ? black : white;
    const Colors variant = orthodox == white ? black : white;
    (orthodox == white ? orthodox_white : orthodox_black) += 1;

    EXPECT_FALSE(is_variant_army(board, orthodox));
    EXPECT_TRUE(is_variant_army(board, variant));
    EXPECT_TRUE(is_orthodox_army(board, orthodox));
    EXPECT_EQ(army_weight(board, variant), army_budget);
    EXPECT_TRUE(army_on_own_half(board, variant));
    // only the orthodox side castles, both ways
    EXPECT_EQ(reparsed.state.castle, orthodox == white ? (wk | wq) : (bk | bq));
    // only the variant side's pawns are listed as unmoved
    EXPECT_EQ(reparsed.state.pawn_unmoved, board.state.bitboards[variant == white ? P : p]);
  }

  EXPECT_GE(orthodox_white, 60);
  EXPECT_GE(orthodox_black, 60);
}
