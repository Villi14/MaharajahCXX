// Endgame knowledge on top of the network (Endgame.h).

#include "../headers/Endgame.h"
#include "../headers/Engine.h"
#include "../headers/ffi/MaharajahFfiInternal.h"
#include "../headers/ffi/maharajah_ffi.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdlib>
#include <string>

using namespace std;
using namespace maharajah;

namespace {

class endgame_search_test_fixture : public testing::Test {
  public:
  void SetUp() override {
    ASSERT_TRUE(mah_init());
  }

  void TearDown() override {
    mah_shutdown();
  }

  static string best_move(const int depth) {
    array<char, 16> move{ };
    return mah_best_move_depth(depth, move.data(), static_cast<int>(move.size())) ? move.data() : "";
  }

  // Plays from `fen` with the side to move at `difficulty` and the defender at full
  // strength; returns the moves until mate, or -1 without mate within `max_moves`.
  static int moves_to_mate(const char* fen, const int difficulty, const int max_moves) {
    EXPECT_TRUE(mah_set_position_fen(fen)) << fen;
    for(int ply{ }; ply < 2 * max_moves; ++ply) {
      mah_set_difficulty_level(ply % 2 == 0 ? difficulty : 5);
      const string move = best_move(8);
      if(move.empty() || !mah_apply_move(move.c_str()))
        return -1;
      if(mah_game_status() == mah_status_checkmate)
        return ply / 2 + 1;
    }
    return -1;
  }
};

// From a game in the app (level 3, 2026-11-06): with rook + knight + pawn against a bare
// king the network scored a king move above promoting, so the depth-capped search
// shuffled the king for over a hundred moves.
TEST_F(endgame_search_test_fixture, a_weak_level_promotes_against_a_bare_king) {
  for(const int difficulty : { 1, 3 }) {
    mah_set_difficulty_level(difficulty);
    ASSERT_TRUE(mah_set_position_fen("8/8/3K4/6r1/6k1/4n3/5p2/8 b - - 0 1"));
    EXPECT_EQ(best_move(10), "f2f1q") << "difficulty " << difficulty;
  }
}

// Level 1 never mated with king + queen: the depth cap is lifted against a bare king.
TEST_F(endgame_search_test_fixture, level_one_mates_a_bare_king) {
  EXPECT_GT(moves_to_mate("8/8/8/4k3/8/8/8/3QK3 w - - 0 1", 1, 30), 0);
  EXPECT_GT(moves_to_mate("8/8/8/4k3/8/8/8/R3K3 w - - 0 1", 1, 40), 0);
}

// the full-strength search against a bare king is for that search only
TEST_F(endgame_search_test_fixture, the_level_comes_back_after_a_bare_king_search) {
  mah_set_difficulty_level(1);
  ASSERT_TRUE(mah_set_position_fen("8/8/8/4k3/8/8/8/3QK3 w - - 0 1"));
  (void)best_move(10);
  EXPECT_EQ(ffi::live_engine().search_config.skill_level, 2);
  EXPECT_EQ(ffi::live_engine().search_config.ui_difficulty, 1);
  // and the next ordinary search is capped again (level 1: depth 1)
  ASSERT_TRUE(mah_set_position_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
  (void)best_move(64);
  EXPECT_EQ(ffi::last_search_result().depth, 1);
}

// A pawn or more up with at most 10 men besides the kings, the side to move plays at
// full strength; the side that is behind keeps its level.
TEST(endgame_full_strength_test, only_the_side_ahead_in_a_small_endgame) {
  Engine engine{ };
  const auto full = [&](const char* fen) {
    EXPECT_TRUE(engine.board.parse_fen(fen)) << fen;
    return Endgame::plays_full_strength(engine.board.state);
  };
  EXPECT_TRUE(full("4k3/8/4K3/4P3/8/8/8/8 w - - 0 1"));           // KPK
  EXPECT_FALSE(full("4k3/8/4K3/4P3/8/8/8/8 b - - 0 1"));          // the defender
  EXPECT_TRUE(full("1K1k4/1P6/8/8/8/8/r7/2R5 w - - 0 1"));        // Lucena
  EXPECT_FALSE(full("4k3/R7/8/4PK2/8/8/8/7r b - - 0 1"));         // Philidor, defender
  EXPECT_FALSE(full("8/p4kpp/8/8/8/8/P4KPP/8 w - - 0 1"));        // equal material
  EXPECT_FALSE(full("r3k3/pppp4/8/8/8/8/PPPPP3/R3K3 w - - 0 1")); // too many men
}

class endgame_eval_test_fixture : public testing::Test {
  public:
  Engine engine{ };

  // Endgame::adjust of a side-to-move score
  int adjust(const char* fen, const int score) {
    EXPECT_TRUE(engine.board.parse_fen(fen)) << fen;
    return Endgame::adjust(engine.board.state, score);
  }
};

TEST_F(endgame_eval_test_fixture, a_bare_king_win_scores_above_the_network) {
  // black to move against a bare king: the score is a known win for black
  EXPECT_GE(adjust("8/8/3K4/6r1/6k1/4n3/5p2/8 b - - 0 1", 0), Endgame::known_win);
  EXPECT_LE(adjust("8/8/3K4/6r1/6k1/4n3/5p2/8 w - - 0 1", 0), -Endgame::known_win);
  // and promoting raises it
  EXPECT_LT(adjust("8/8/3K4/6r1/6k1/4n3/8/5q2 w - - 0 1", 0), adjust("8/8/3K4/6r1/5k2/4n3/5p2/8 w - - 0 1", 0));
}

TEST_F(endgame_eval_test_fixture, no_bare_king_win_without_mating_pieces) {
  // a lone knight or bishop, or only pawns, keep the network's score
  EXPECT_EQ(adjust("8/8/8/4k3/8/8/8/1N2K3 w - - 0 1", 123), 123 * 8 / 64);
  EXPECT_EQ(adjust("8/8/8/4k3/8/8/4P3/4K3 w - - 0 1", 456), 456);
}

TEST_F(endgame_eval_test_fixture, drawish_material_is_scaled_towards_a_draw) {
  // two knights against a bare king
  EXPECT_LE(abs(adjust("8/8/8/4k3/8/8/8/1N2KN2 w - - 0 1", 472)), 20);
  // rook against bishop
  EXPECT_LE(abs(adjust("8/8/8/4k3/8/3b4/8/R3K3 w - - 0 1", 328)), 50);
  // h pawn with the light-squared bishop, the defending king on h8
  EXPECT_LE(abs(adjust("7k/8/7K/7P/8/8/8/5B2 w - - 0 1", 763)), 30);
}

TEST_F(endgame_eval_test_fixture, winning_material_is_not_scaled) {
  // queen against rook
  EXPECT_EQ(adjust("8/8/8/3k4/8/8/2r5/3QK3 w - - 0 1", 1700), 1700);
  // the right bishop for the h pawn
  EXPECT_EQ(adjust("7k/8/7K/7P/8/8/8/4B3 w - - 0 1", 700), 700);
  // a bishop against five pawns: the pawns' side is winning and keeps its score
  EXPECT_EQ(adjust("8/8/8/4k3/1ppppp2/8/8/1B2K3 b - - 0 1", 600), 600);
}

} // namespace
