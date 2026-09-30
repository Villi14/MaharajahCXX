// Strength profiles, staged NNUE state and transposition-table round trips, ported
// from MaharajahC's engine_config_smoke.c and transposition_smoke.c.

#include "../headers/Engine.h"
#include "../headers/EngineConfig.h"
#include "../headers/Nnue.h"
#include "../headers/Transposition.h"
#include "gtest/gtest.h"

#include <array>

using namespace std;
using namespace maharajah;

// engine_config_smoke.c

TEST(engine_config_test, default_profile_is_full_strength_classic) {
  const SearchConfig config = SearchConfig::for_difficulty(SearchConfig::max_difficulty);
  EXPECT_EQ(config.ui_difficulty, 5);
  EXPECT_EQ(config.skill_level, 10);
  EXPECT_EQ(config.max_depth_cap, Limits::max_ply);
  EXPECT_EQ(EvalConfig{ }.eval_mode, EvalMode::classic);
}

TEST(engine_config_test, difficulty_is_clamped_and_mapped_to_skill) {
  SearchConfig config = SearchConfig::for_difficulty(0);
  EXPECT_EQ(config.ui_difficulty, 1);
  EXPECT_EQ(config.skill_level, 2);

  config = SearchConfig::for_difficulty(99);
  EXPECT_EQ(config.ui_difficulty, 5);
  EXPECT_EQ(config.skill_level, 10);
}

TEST(engine_config_test, skill_is_clamped_and_clears_the_difficulty) {
  SearchConfig config = SearchConfig::for_skill(0);
  EXPECT_EQ(config.ui_difficulty, 0);
  EXPECT_EQ(config.skill_level, 1);
  EXPECT_EQ(config.max_depth_cap, 1);

  config = SearchConfig::for_skill(99);
  EXPECT_EQ(config.ui_difficulty, 0);
  EXPECT_EQ(config.skill_level, 10);
  EXPECT_EQ(config.max_depth_cap, Limits::max_ply);
}

TEST(engine_config_test, nnue_weights_load_from_memory_but_the_backend_stays_offline) {
  constexpr array<unsigned char, 4> weights{ 0x4e, 0x4e, 0x55, 0x45 };
  Nnue nnue;
  EXPECT_FALSE(nnue.has_loaded_weights());

  ASSERT_TRUE(nnue.load_weights_from_bytes(weights.data(), weights.size(), "memory-v1"));
  EXPECT_TRUE(nnue.has_loaded_weights());
  EXPECT_FALSE(Nnue::backend_ready());
  EXPECT_EQ(nnue.weights_version(), "memory-v1");
  EXPECT_EQ(nnue.loaded_path(), "<memory>");

  nnue.unload_weights();
  EXPECT_FALSE(nnue.has_loaded_weights());
}

// transposition_smoke.c

class transposition_test_fixture : public testing::Test {
  public:
  Engine engine{ };
  Board& board = engine.board;
  TranspositionTable table{ };

  // stores an exact score, then reads it back both directly and after rebuilding the
  // position from the FEN (the hash must not depend on how the position was reached)
  void expect_exact_roundtrip(const char* fen, const int score, const int depth) {
    board.parse_fen(fen);
    table.write(board.state.hash_key, score, depth, HashFlag::exact, 0);
    EXPECT_EQ(table.read(board.state.hash_key, -Scores::infinity, Scores::infinity, depth, 0), score) << fen;

    board.parse_fen(fen);
    EXPECT_EQ(table.read(board.state.hash_key, -Scores::infinity, Scores::infinity, depth, 0), score) << fen;
  }
};

TEST_F(transposition_test_fixture, compound_piece_positions_round_trip) {
  expect_exact_roundtrip("k7/8/8/8/3A4/8/8/7K w - - 0 1 ", 123, 4);
  expect_exact_roundtrip("k7/8/8/8/3C4/8/8/7K w - - 0 1 ", -77, 5);
  expect_exact_roundtrip("k7/8/8/8/3M4/8/8/7K w - - 0 1 ", 314, 6);
}

TEST_F(transposition_test_fixture, clear_removes_entries) {
  expect_exact_roundtrip("k7/8/8/8/3M4/8/8/7K w - - 0 1 ", 314, 6);
  table.clear();
  board.parse_fen("k7/8/8/8/3M4/8/8/7K w - - 0 1 ");
  EXPECT_EQ(table.read(board.state.hash_key, -Scores::infinity, Scores::infinity, 6, 0), TranspositionTable::no_entry);
}

// the lockless table packs score, depth and flag into one word
TEST_F(transposition_test_fixture, packed_entries_keep_sign_mate_scores_and_flags) {
  constexpr u64 key{ 0x123456789ABCDEF0 };
  constexpr int mated_in_three{ -Scores::mate_value + 3 };

  // mate scores are stored relative to the node and restored at the reading ply
  table.write(key, mated_in_three, 7, HashFlag::exact, 3);
  EXPECT_EQ(table.read(key, -Scores::infinity, Scores::infinity, 7, 3), mated_in_three);
  EXPECT_EQ(table.read(key, -Scores::infinity, Scores::infinity, 7, 1), mated_in_three - 2);
  EXPECT_EQ(table.read(key, -Scores::infinity, Scores::infinity, 8, 3), TranspositionTable::no_entry);
  EXPECT_EQ(table.read(key ^ 1, -Scores::infinity, Scores::infinity, 7, 3), TranspositionTable::no_entry);

  table.write(key, -250, 2, HashFlag::alpha, 0);
  EXPECT_EQ(table.read(key, -200, 0, 2, 0), -200);
  EXPECT_EQ(table.read(key, -300, 0, 2, 0), TranspositionTable::no_entry);

  table.write(key, 250, 2, HashFlag::beta, 0);
  EXPECT_EQ(table.read(key, 0, 200, 2, 0), 200);
  EXPECT_EQ(table.read(key, 0, 300, 2, 0), TranspositionTable::no_entry);
}
