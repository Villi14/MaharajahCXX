// Draw-rule regressions ported from MaharajahC's draw_rules_smoke.c. MaharajahC has
// separate threefold/fivefold/fifty/seventy-five predicates; here they are one
// check in Search::is_draw (see SearchTests.cpp), so these tests pin its inputs
// (repetition count, halfmove clock, standard_rules) where the C test did.

#include "../headers/CustomSetup.h"
#include "../headers/Search.h"
#include "TestAccess.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class draw_rules_test_fixture : public testing::Test {
  public:
  Engine engine{ };
  Board& board = engine.board;

  void play(const char* move) {
    ASSERT_TRUE(engine.apply_move(move)) << move;
  }

  void play_all(const initializer_list<const char*> moves) {
    for(const char* move : moves)
      play(move);
  }

  bool is_draw() {
    return Search(engine).is_draw();
  }

  bool is_insufficient_material(const char* fen) {
    board.parse_fen(fen);
    return Search(engine).is_insufficient_material();
  }
};

TEST_F(draw_rules_test_fixture, knight_shuffle_counts_repetitions) {
  board.parse_fen("7k/8/8/8/8/8/6N1/7K w - - 0 1 ");
  EXPECT_EQ(board.repetition_count(), 1);

  play_all({ "g2f4", "h8g8", "f4g2", "g8h8", "g2f4", "h8g8" });
  EXPECT_EQ(board.repetition_count(), 2);

  // at the root, going back to g2 would repeat the position; a king move would not
  Search search(engine);
  const int repeating = board.parse_move("f4g2");
  const int fresh = board.parse_move("h1g1");
  ASSERT_NE(repeating, 0);
  ASSERT_NE(fresh, 0);
  EXPECT_TRUE(SearchTestAccess::root_move_repeats_position(search, repeating));
  EXPECT_FALSE(SearchTestAccess::root_move_repeats_position(search, fresh));

  play_all({ "f4g2", "g8h8" });
  EXPECT_EQ(board.repetition_count(), 3);

  play_all({ "g2f4", "h8g8", "f4g2", "g8h8", "g2f4", "h8g8", "f4g2", "g8h8" });
  EXPECT_GE(board.repetition_count(), 5);
}

TEST_F(draw_rules_test_fixture, halfmove_clock_reaches_the_fifty_and_seventy_five_move_marks) {
  board.parse_fen("7k/8/8/8/8/8/6N1/7K w - - 99 1 ");
  EXPECT_TRUE(board.state.standard_rules);
  play("g2f4");
  EXPECT_EQ(board.state.halfmove, 100);

  board.parse_fen("7k/8/8/8/8/8/6N1/7K w - - 149 1 ");
  play("g2f4");
  EXPECT_EQ(board.state.halfmove, 150);
}

TEST_F(draw_rules_test_fixture, pawn_move_resets_the_halfmove_clock) {
  board.parse_fen("7k/8/8/8/8/8/4P3/7K w - - 99 1 ");
  play("e2e3");
  EXPECT_EQ(board.state.halfmove, 0);
}

TEST_F(draw_rules_test_fixture, halfmove_rules_are_off_with_compound_pieces) {
  board.parse_fen("7k/8/8/8/3M4/8/8/7K w - - 150 1 ");
  EXPECT_FALSE(board.state.standard_rules);
  EXPECT_FALSE(is_draw());
}

TEST_F(draw_rules_test_fixture, halfmove_rules_are_off_in_a_custom_position) {
  ASSERT_TRUE(generate_custom_position(board, -1, 7U));
  EXPECT_FALSE(board.state.standard_rules);
  board.state.halfmove = 150;
  EXPECT_FALSE(is_draw());
}

TEST_F(draw_rules_test_fixture, insufficient_material) {
  EXPECT_TRUE(is_insufficient_material("7k/8/8/8/8/8/8/7K w - - 0 1 ")); // K v K
  EXPECT_TRUE(is_insufficient_material("7k/8/8/8/8/8/8/6BK w - - 0 1 ")); // K+B v K
  EXPECT_TRUE(is_insufficient_material("7k/8/8/8/8/8/8/6NK w - - 0 1 ")); // K+N v K
  // Bg1 and bf2 both on dark squares: dead position
  EXPECT_TRUE(is_insufficient_material("7k/8/8/8/8/8/5b2/6BK w - - 0 1 "));

  // helpmates still exist
  EXPECT_FALSE(is_insufficient_material("7k/8/8/8/8/8/8/5NNK w - - 0 1 ")); // K+N+N v K
  EXPECT_FALSE(is_insufficient_material("7k/8/8/8/8/8/6b1/6BK w - - 0 1 ")); // opposite-coloured bishops
  EXPECT_FALSE(is_insufficient_material("7k/8/8/8/8/8/6n1/6BK w - - 0 1 ")); // K+B v K+N
  EXPECT_FALSE(is_insufficient_material("7k/8/8/8/8/8/6n1/6NK w - - 0 1 ")); // K+N v K+N
  EXPECT_FALSE(is_insufficient_material("7k/8/8/8/8/8/6q1/6BK w - - 0 1 ")); // a queen
}
