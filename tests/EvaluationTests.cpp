// Evaluation and SEE regressions ported from MaharajahC's evaluate_safety_smoke.c
// and see_smoke.c. FENs are kept verbatim, including the trailing spaces.

#include "../headers/Engine.h"
#include "../headers/Evaluator.h"
#include "../headers/Move.h"
#include "../headers/See.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class evaluation_test_fixture : public testing::Test {
  public:
  Engine engine{ }; // initialises the attack tables
  Board& board = engine.board;

  int evaluate(const char* fen) {
    board.parse_fen(fen);
    return Evaluator::evaluate(board.state);
  }

  int see(const char* fen, const char* move) {
    board.parse_fen(fen);
    const int parsed = board.parse_move(move);
    EXPECT_NE(parsed, 0) << move;
    return see_evaluate(board, parsed);
  }
};

// evaluate_safety_smoke.c

TEST_F(evaluation_test_fixture, hanging_piece_evaluates_worse_than_a_safe_one) {
  // the piece on d4 is attacked by the e5 pawn; on d3 it is safe
  for(const char piece : { 'Q', 'R', 'M', 'C', 'A' }) {
    const string hanging = "7k/8/8/4p3/3" + string(1, piece) + "4/8/8/K7 w - - 0 1 ";
    const string safe = "7k/8/8/4p3/8/3" + string(1, piece) + "4/8/K7 w - - 0 1 ";
    EXPECT_LT(evaluate(hanging.c_str()), evaluate(safe.c_str())) << piece;
  }
}

TEST_F(evaluation_test_fixture, poisoned_queen_evaluates_worse_than_a_safe_one) {
  EXPECT_LT(evaluate("7k/8/8/8/8/8/1q6/K1B5 b - - 0 1 "), evaluate("7k/8/8/8/8/1q6/8/K1B5 b - - 0 1 "));
}

TEST_F(evaluation_test_fixture, attacked_but_defended_piece_evaluates_worse_than_a_safer_one) {
  EXPECT_LT(evaluate("7k/8/8/4p3/3Q4/2B5/8/K7 w - - 0 1 "), evaluate("7k/8/8/4p3/8/2BQ4/8/K7 w - - 0 1 "));
  EXPECT_LT(evaluate("7k/8/8/4p3/3C4/2B5/8/K7 w - - 0 1 "), evaluate("7k/8/8/4p3/8/2BC4/8/K7 w - - 0 1 "));
}

TEST_F(evaluation_test_fixture, black_passed_pawn_bonus_grows_as_it_advances) {
  EXPECT_LT(evaluate("4k3/4p3/8/8/8/8/8/4K3 b - - 0 1 "), evaluate("4k3/8/8/8/8/8/4p3/4K3 b - - 0 1 "));
}

TEST_F(evaluation_test_fixture, mirrored_pawn_positions_evaluate_the_same) {
  EXPECT_EQ(evaluate("4k3/4P3/8/8/8/8/8/4K3 w - - 0 1 "), evaluate("4k3/8/8/8/8/8/4p3/4K3 b - - 0 1 "));
  EXPECT_EQ(evaluate("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1 "), evaluate("4k3/4p3/8/8/8/8/8/4K3 b - - 0 1 "));
}

// see_smoke.c

TEST_F(evaluation_test_fixture, see_knight_takes_undefended_bishop_wins) {
  // see_smoke.c has "3N3" on rank 5, which MaharajahC's lenient parser pads; the
  // strict parser here rejects it
  board.parse_fen("8/1k6/1b6/3N4/8/8/8/4K3 w - - 0 1 ");
  const int move = board.parse_move("d5b6");
  ASSERT_NE(move, 0);
  EXPECT_TRUE(Move::get_move_capture(move));
  EXPECT_GT(see_evaluate(board, move), 0);
}

TEST_F(evaluation_test_fixture, see_queen_takes_rook_defended_pawn_loses) {
  EXPECT_LT(see("3r1k2/3p4/3Q4/8/8/8/8/4K3 w - - 0 1 ", "d6d7"), 0);
}

TEST_F(evaluation_test_fixture, see_lets_the_capturer_stand_pat) {
  // Rxa5 wins the queen; after ...bxa5 White need not continue Qxa5 Rxa5
  EXPECT_GT(see("r6k/8/1p6/q7/R7/Q7/8/7K w - - 0 1 ", "a4a5"), 0);
}
