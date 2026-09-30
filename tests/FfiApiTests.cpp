// The C interface (mah_*), ported from MaharajahC's ffi_api_smoke.c. FENs are kept
// verbatim, including the trailing spaces.

#include "../headers/Search.h"
#include "../headers/ffi/MaharajahFfiInternal.h"
#include "../headers/ffi/maharajah_ffi.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

using namespace std;
using namespace maharajah;

namespace {

// occupied squares of FEN row `row` (0 = rank 8)
int row_occupancy(const string_view fen, const int row) {
  int current{ };
  int occupied{ };
  for(const char c : fen.substr(0, fen.find(' '))) {
    if(c == '/')
      ++current;
    else if(current == row && (c < '1' || c > '8'))
      ++occupied;
  }
  return occupied;
}

// Each army fills its ranks from the home rank inwards: no piece stands on a rank
// beyond one that is not full.
bool respects_row_progression(const string_view fen) {
  const auto check = [&fen](const int first, const int last, const int step) {
    bool found_incomplete{ };
    for(int row = first; row != last + step; row += step) {
      const int occupied = row_occupancy(fen, row);
      if(found_incomplete && occupied > 0)
        return false;
      if(occupied != 8)
        found_incomplete = true;
    }
    return true;
  };
  return check(7, 4, -1) && check(0, 3, 1);
}

} // namespace

class ffi_api_test_fixture : public testing::Test {
  public:
  void SetUp() override {
    ASSERT_TRUE(mah_init());
  }

  void TearDown() override {
    mah_shutdown();
  }

  static string get_fen(const int fullmove_number) {
    array<char, 128> fen{ };
    EXPECT_TRUE(mah_get_fen(fen.data(), static_cast<int>(fen.size()), fullmove_number));
    return fen.data();
  }

  // best move at `depth`, or "" if the engine found none
  static string best_move(const int depth) {
    array<char, 16> move{ };
    return mah_best_move_depth(depth, move.data(), static_cast<int>(move.size())) ? move.data() : "";
  }

  static string timed_best_move(const int movetime_ms) {
    array<char, 16> move{ };
    return mah_best_move_time(movetime_ms, move.data(), static_cast<int>(move.size())) ? move.data() : "";
  }

  static string nnue_info() {
    array<char, 512> info{ };
    EXPECT_TRUE(mah_get_nnue_info(info.data(), static_cast<int>(info.size())));
    return info.data();
  }

  static string eval_status() {
    array<char, 64> status{ };
    EXPECT_TRUE(mah_get_eval_status(status.data(), static_cast<int>(status.size())));
    return status.data();
  }

  static bool contains(const string& text, const string_view part) {
    return text.find(part) != string::npos;
  }
};

TEST_F(ffi_api_test_fixture, apply_move_rejects_garbage_and_illegal_moves) {
  ASSERT_TRUE(mah_set_position_startpos());
  EXPECT_TRUE(mah_apply_move("e2e4"));
  EXPECT_FALSE(mah_apply_move("e7e5x"));
  EXPECT_FALSE(mah_apply_move("e7e3"));
}

TEST_F(ffi_api_test_fixture, get_fen_reports_en_passant_castling_and_clocks) {
  ASSERT_TRUE(mah_set_position_startpos());
  ASSERT_TRUE(mah_apply_move("e2e4"));
  EXPECT_EQ(get_fen(1), "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1 -");

  // a king move drops both castling rights of its side and increments the clock;
  // the caller's fullmove number is echoed back
  ASSERT_TRUE(mah_set_position_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 5 9 "));
  ASSERT_TRUE(mah_apply_move("e1f1"));
  EXPECT_EQ(get_fen(9), "r3k2r/8/8/8/8/8/8/R4K1R b kq - 6 9 -");

  array<char, 8> small{ };
  EXPECT_FALSE(mah_get_fen(small.data(), static_cast<int>(small.size()), 1));
}

TEST_F(ffi_api_test_fixture, get_fen_round_trips_the_unmoved_pawn_field) {
  ASSERT_TRUE(mah_set_position_fen("8/5p2/7p/1K6/4a3/5k2/8/8 b - - 25 44 Vv h6"));
  EXPECT_EQ(get_fen(44), "8/5p2/7p/1K6/4a3/5k2/8/8 b - - 25 44 Vv h6");

  // once the h6 pawn moves its right is spent and field 8 collapses to "-"
  ASSERT_TRUE(mah_apply_move("h6h5"));
  const string fen = get_fen(45);
  EXPECT_EQ(fen, "8/5p2/8/1K5p/4a3/5k2/8/8 w - - 0 45 Vv -");
  EXPECT_TRUE(mah_set_position_fen(fen.c_str()));
}

TEST_F(ffi_api_test_fixture, fen_without_one_king_per_side_is_rejected) {
  EXPECT_FALSE(mah_set_position_fen("8/8/8/8/8/8/8/8 w - - 0 1 "));
  EXPECT_FALSE(mah_set_position_fen("kk6/8/8/8/8/8/8/K7 w - - 0 1 "));
  // the engine is left on a usable position
  EXPECT_EQ(mah_game_status(), mah_status_ongoing);
}

TEST_F(ffi_api_test_fixture, rules_profiles_control_the_halfmove_draw_rules) {
  EXPECT_TRUE(mah_set_position_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 "));

  // standard rules cannot be forced onto a board with compound pieces
  ASSERT_TRUE(mah_set_position_fen_with_rules("7k/8/8/8/3M4/8/8/7K w - - 150 1 ", mah_rules_standard));
  Engine& live = ffi::live_engine();
  EXPECT_FALSE(live.board.state.standard_rules);
  EXPECT_FALSE(Search(live).is_draw());

  ASSERT_TRUE(mah_set_position_fen_with_rules("7k/8/8/8/8/8/6N1/7K w - - 150 1 ", mah_rules_variant));
  EXPECT_FALSE(live.board.state.standard_rules);
}

TEST_F(ffi_api_test_fixture, amazon_promotion_under_the_variant_profile) {
  ASSERT_TRUE(mah_set_position_fen_with_rules("7k/P7/8/8/8/8/8/K7 w - - 0 1 ", mah_rules_variant));
  EXPECT_TRUE(mah_apply_move("a7a8m"));

  ASSERT_TRUE(mah_set_position_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 "));
  EXPECT_FALSE(mah_apply_move("a7a8mx"));
}

TEST_F(ffi_api_test_fixture, pawns_on_the_edge_ranks_are_accepted_and_searched) {
  ASSERT_TRUE(mah_set_position_fen("p3k3/8/8/8/8/8/8/4K2P w - - 0 1 "));
  EXPECT_NE(best_move(1), "");
}

TEST_F(ffi_api_test_fixture, best_move_depth_finds_the_archbishop_capture) {
  ASSERT_TRUE(mah_set_position_fen("k7/5q2/8/4A3/8/8/8/K7 w - - 0 1 "));
  EXPECT_EQ(best_move(1), "e5f7");
}

TEST_F(ffi_api_test_fixture, game_status_and_no_move_in_stalemate_and_checkmate) {
  ASSERT_TRUE(mah_set_position_fen("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1 "));
  EXPECT_EQ(mah_game_status(), mah_status_stalemate);
  EXPECT_EQ(best_move(1), "");
  EXPECT_EQ(timed_best_move(10), "");

  ASSERT_TRUE(mah_set_position_fen("k7/1M6/2K5/8/8/8/8/8 b - - 0 1 "));
  EXPECT_EQ(mah_game_status(), mah_status_checkmate);
  EXPECT_EQ(best_move(1), "");
  EXPECT_EQ(timed_best_move(10), "");

  // the position reached by Be5xh2# in the online match that hung
  ASSERT_TRUE(mah_set_position_fen("2kr3r/1qp2p2/p1p1p3/3p2B1/4P3/P1N2b2/1PP2P1b/1RQ2RK1 w - - 0 20 -"));
  EXPECT_EQ(mah_game_status(), mah_status_checkmate);

  ASSERT_TRUE(mah_set_position_startpos());
  EXPECT_EQ(mah_game_status(), mah_status_ongoing);
}

TEST_F(ffi_api_test_fixture, timed_search_returns_a_move) {
  ASSERT_TRUE(mah_set_position_startpos());
  EXPECT_NE(timed_best_move(10), "");
}

TEST_F(ffi_api_test_fixture, search_avoids_a_repeating_move) {
  ASSERT_TRUE(mah_set_position_fen("7k/8/8/8/8/8/6N1/7K w - - 0 1 "));
  for(const char* move : { "g2f4", "h8g8", "f4g2", "g8h8", "g2f4", "h8g8" })
    ASSERT_TRUE(mah_apply_move(move)) << move;
  const string move = best_move(2);
  EXPECT_NE(move, "");
  EXPECT_NE(move, "f4g2");

  array<char, 4> small{ };
  EXPECT_FALSE(mah_best_move_depth(1, small.data(), static_cast<int>(small.size())));
}

TEST_F(ffi_api_test_fixture, difficulty_and_skill_select_the_search_profile) {
  EXPECT_TRUE(mah_set_hash_mb(4));

  ASSERT_TRUE(mah_set_difficulty_level(3));
  string info = nnue_info();
  EXPECT_TRUE(contains(info, "\"uiDifficulty\":3")) << info;
  EXPECT_TRUE(contains(info, "\"skillLevel\":6")) << info;

  // tuning the skill directly clears the UI difficulty
  ASSERT_TRUE(mah_set_skill_level(3));
  info = nnue_info();
  EXPECT_TRUE(contains(info, "\"uiDifficulty\":0")) << info;
  EXPECT_TRUE(contains(info, "\"skillLevel\":3")) << info;

  ASSERT_TRUE(mah_set_difficulty_level(0));
  info = nnue_info();
  EXPECT_TRUE(contains(info, "\"uiDifficulty\":1")) << info;
  EXPECT_TRUE(contains(info, "\"skillLevel\":2")) << info;

  ASSERT_TRUE(mah_set_skill_level(99));
  info = nnue_info();
  EXPECT_TRUE(contains(info, "\"uiDifficulty\":0")) << info;
  EXPECT_TRUE(contains(info, "\"skillLevel\":10")) << info;

  ASSERT_TRUE(mah_set_position_startpos());
  EXPECT_NE(best_move(6), "");
}

TEST_F(ffi_api_test_fixture, nnue_mode_is_requested_but_classic_stays_active) {
  ASSERT_TRUE(mah_set_eval_mode(1));
  EXPECT_EQ(eval_status(), "classic_ready");

  const string info = nnue_info();
  EXPECT_TRUE(contains(info, "\"status\":\"classic_ready\"")) << info;
  EXPECT_TRUE(contains(info, "\"requestedEvalMode\":\"nnue\"")) << info;
  EXPECT_TRUE(contains(info, "\"activeEvalMode\":\"classic\"")) << info;
  EXPECT_TRUE(contains(info, "\"usingClassicFallback\":1")) << info;
  EXPECT_TRUE(contains(info, "\"weightsLoaded\":0")) << info;

  ASSERT_TRUE(mah_set_position_fen("k7/5q2/8/4A3/8/8/8/K7 w - - 0 1 "));
  EXPECT_EQ(best_move(1), "e5f7");

  ASSERT_TRUE(mah_set_eval_mode(0));
  // an invalid mode falls back to classic
  ASSERT_TRUE(mah_set_eval_mode(99));
  EXPECT_EQ(eval_status(), "classic_ready");
}

TEST_F(ffi_api_test_fixture, weights_load_and_unload) {
  const string path = testing::TempDir() + "ffi_api_test_weights.nnue";
  {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    std::fputs("stub nnue weights\n", file);
    std::fclose(file);
  }

  EXPECT_TRUE(mah_load_weights(path.c_str()));
  EXPECT_TRUE(mah_unload_weights());
  EXPECT_TRUE(mah_load_weights_from_bytes(reinterpret_cast<const unsigned char*>("NNUE"), 4, "memory-v1"));
  EXPECT_TRUE(mah_unload_weights());
  std::remove(path.c_str());

  const string info = nnue_info();
  EXPECT_TRUE(contains(info, "\"weightsLoaded\":0")) << info;
  EXPECT_TRUE(contains(info, "\"uiDifficulty\":5")) << info;
  EXPECT_TRUE(contains(info, "\"skillLevel\":10")) << info;
}

TEST_F(ffi_api_test_fixture, generated_custom_position_is_well_formed_and_playable) {
  array<char, 128> fen{ };
  ASSERT_TRUE(mah_generate_custom_position_fen(-1, 7U, fen.data(), static_cast<int>(fen.size())));
  ASSERT_NE(fen[0], '\0');
  EXPECT_TRUE(respects_row_progression(fen.data())) << fen.data();
  ASSERT_TRUE(mah_set_position_fen(fen.data()));
  EXPECT_NE(best_move(1), "");

  array<char, 8> small{ };
  EXPECT_FALSE(mah_generate_custom_position_fen(0, 7U, small.data(), static_cast<int>(small.size())));
}

TEST_F(ffi_api_test_fixture, king_capture_is_never_a_legal_move) {
  ASSERT_TRUE(mah_set_position_fen("4k3/8/8/4M3/8/8/8/4K3 w - - 0 1 "));
  EXPECT_FALSE(mah_apply_move("e5e8"));
  EXPECT_NE(best_move(1), "");
}

TEST_F(ffi_api_test_fixture, set_threads_searches_with_helper_threads) {
  EXPECT_TRUE(mah_set_threads(4));
  EXPECT_EQ(maharajah::ffi::live_engine().threads, 4);
  EXPECT_TRUE(mah_set_threads(0));
  EXPECT_EQ(maharajah::ffi::live_engine().threads, 1);
  EXPECT_TRUE(mah_set_threads(1000));
  EXPECT_EQ(maharajah::ffi::live_engine().threads, 64);

  // amazon smothered mate, see SearchTests
  ASSERT_TRUE(mah_set_threads(4));
  ASSERT_TRUE(mah_set_position_fen("6rk/6pp/8/8/2M5/8/8/K7 w - - 0 1"));
  EXPECT_EQ(best_move(5), "c4f7");
  ASSERT_TRUE(mah_set_position_startpos());
  EXPECT_NE(timed_best_move(100), "");
}
