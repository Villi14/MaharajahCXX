// Best-move regressions through the C interface, ported from MaharajahC's
// search_sanity_smoke.c. FENs are kept verbatim, including the trailing spaces.

#include "../headers/ffi/MaharajahFfiInternal.h"
#include "../headers/ffi/maharajah_ffi.h"
#include "gtest/gtest.h"

#include <array>
#include <string>

using namespace std;
using namespace maharajah;

class search_sanity_test_fixture : public testing::Test {
  public:
  void SetUp() override {
    ASSERT_TRUE(mah_init());
  }

  void TearDown() override {
    mah_shutdown();
  }

  // best move at `depth`, or "" if the engine found none
  static string best_move(const char* fen, const int depth, const int rules = mah_rules_auto) {
    array<char, 16> move{ };
    EXPECT_TRUE(mah_set_position_fen_with_rules(fen, rules)) << fen;
    return mah_best_move_depth(depth, move.data(), static_cast<int>(move.size())) ? move.data() : "";
  }
};

TEST_F(search_sanity_test_fixture, no_move_in_checkmate_or_stalemate) {
  EXPECT_EQ(best_move("k7/1M6/2K5/8/8/8/8/8 b - - 0 1 ", 1), "");
  EXPECT_EQ(best_move("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1 ", 1), "");
}

TEST_F(search_sanity_test_fixture, prefers_the_amazon_promotion_under_variant_rules) {
  EXPECT_EQ(best_move("7k/P7/8/8/8/8/8/K7 w - - 0 1 ", 1, mah_rules_variant), "a7a8m");
}

TEST_F(search_sanity_test_fixture, compound_pieces_capture_the_queen) {
  EXPECT_EQ(best_move("k7/5q2/8/4A3/8/8/8/K7 w - - 0 1 ", 1), "e5f7");
  EXPECT_EQ(best_move("7k/8/8/3C4/8/8/3q4/K7 w - - 0 1 ", 1), "d5d2");
  EXPECT_EQ(best_move("k7/8/8/3M4/8/8/8/K6q w - - 0 1 ", 2), "d5h1");
}

TEST_F(search_sanity_test_fixture, escapes_check_by_capturing_safely) {
  EXPECT_EQ(best_move("6k1/8/8/8/8/8/6q1/6RK w - - 0 1 ", 1), "h1g2");
  EXPECT_EQ(best_move("6k1/8/8/8/3M4/8/6q1/6RK w - - 0 1 ", 2), "h1g2");
}

TEST_F(search_sanity_test_fixture, does_not_grab_the_poisoned_pawn_and_punishes_it) {
  EXPECT_NE(best_move("rnb1kbnr/ppp1pppp/8/1q6/8/5N2/PPPP1PPP/RNBQK2R b KQkq - 1 4 ", 1), "b5b2");
  EXPECT_EQ(best_move("rnb1kbnr/ppp1pppp/8/8/8/5N2/PqPP1PPP/RNBQK2R w KQkq - 0 5 ", 1), "c1b2");
}

TEST_F(search_sanity_test_fixture, captures_hanging_compound_pieces) {
  EXPECT_EQ(best_move("7k/8/8/8/8/8/1m6/K1B5 w - - 0 1 ", 1), "c1b2");
  EXPECT_EQ(best_move("7k/8/7c/8/8/8/8/K1B5 w - - 0 1 ", 1), "c1h6");
  EXPECT_EQ(best_move("7k/8/8/8/8/8/1a6/K1B5 w - - 0 1 ", 1), "c1b2");
}

TEST_F(search_sanity_test_fixture, weak_timed_search_does_not_hang_the_queen) {
  ASSERT_TRUE(mah_set_position_fen("rnbqkb1r/pp3ppp/4N3/2pn4/8/2N5/PPPP1PPP/R1BQKB1R b KQkq - 0 6 "));
  ASSERT_TRUE(mah_set_difficulty_level(3));
  ffi::live_engine().random.state = 3U;

  array<char, 16> move{ };
  ASSERT_TRUE(mah_best_move_time(600, move.data(), static_cast<int>(move.size())));
  EXPECT_NE(string(move.data()), "b8c6");
}
