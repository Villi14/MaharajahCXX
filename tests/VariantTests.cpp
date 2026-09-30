#include "../headers/Bitboard.h"
#include "../headers/CustomSetup.h"
#include "../headers/Engine.h"
#include "../headers/Zobrist.h"
#include "../headers/ffi/maharajah_ffi.h"
#include "gtest/gtest.h"

#include <array>
#include <sstream>
#include <string>

using namespace std;
using namespace maharajah;

namespace {

bool has_move(const Board& board, const string& move) {
  return board.parse_move(move) != 0;
}

} // namespace

class variant_test_fixture : public testing::Test {
  public:
  Engine engine{ };
  Board& board = engine.board;
};

TEST_F(variant_test_fixture, compound_pieces_combine_their_components) {
  board.parse_fen("4k3/8/8/8/3A4/8/8/4K3 w - - 0 1");
  EXPECT_TRUE(has_move(board, "d4a7")); // diagonal
  EXPECT_TRUE(has_move(board, "d4e6")); // knight hop
  EXPECT_FALSE(has_move(board, "d4d7")); // no file move

  board.parse_fen("4k3/8/8/8/3C4/8/8/4K3 w - - 0 1");
  EXPECT_TRUE(has_move(board, "d4d8"));
  EXPECT_TRUE(has_move(board, "d4c6"));
  EXPECT_FALSE(has_move(board, "d4e5"));

  board.parse_fen("4k3/8/8/8/3M4/8/8/4K3 w - - 0 1");
  EXPECT_TRUE(has_move(board, "d4h8"));
  EXPECT_TRUE(has_move(board, "d4d1"));
  EXPECT_TRUE(has_move(board, "d4f5"));
}

TEST_F(variant_test_fixture, compound_pieces_give_check) {
  board.parse_fen("4k3/8/3a4/8/8/8/8/4K3 w - - 0 1");
  // the archbishop's knight move covers e4... and its diagonal covers f4
  EXPECT_TRUE(board.is_square_attacked(e4, black));
  EXPECT_TRUE(board.is_square_attacked(f4, black));
  EXPECT_FALSE(board.is_square_attacked(d1, black));
}

TEST_F(variant_test_fixture, fen_derives_variant_rules_from_compound_material) {
  board.parse_fen("r3k2r/8/8/8/8/8/8/R2AK2R w KQkq - 0 1");
  EXPECT_FALSE(board.state.standard_rules);
  EXPECT_TRUE(board.state.side_variant[white]);
  EXPECT_TRUE(board.state.side_variant[black]);
  // variant sides never castle
  EXPECT_EQ(board.state.castle, 0);
}

TEST_F(variant_test_fixture, fen_field_seven_sets_rules_per_side) {
  board.parse_fen("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1 v");
  EXPECT_FALSE(board.state.side_variant[white]);
  EXPECT_TRUE(board.state.side_variant[black]);
  EXPECT_EQ(board.state.castle, wk | wq);
  EXPECT_TRUE(has_move(board, "e1g1"));
}

TEST_F(variant_test_fixture, variant_pawns_double_step_from_any_rank) {
  board.parse_fen("4k3/8/8/8/8/3P4/8/4K3 w - - 0 1 V");
  EXPECT_TRUE(has_move(board, "d3d5"));
  board.parse_fen("4k3/8/8/8/8/3P4/8/4K3 w - - 0 1 -");
  EXPECT_FALSE(has_move(board, "d3d5"));
}

TEST_F(variant_test_fixture, fen_field_eight_tracks_unmoved_pawns) {
  board.parse_fen("4k3/8/8/8/8/2P1P3/8/4K3 w - - 0 1 V c3");
  EXPECT_TRUE(board.state.has_pawn_state);
  EXPECT_TRUE(has_move(board, "c3c5"));
  EXPECT_FALSE(has_move(board, "e3e5"));

  ASSERT_TRUE(engine.apply_move("c3c4"));
  EXPECT_FALSE(get_bit(board.state.pawn_unmoved, c4));
}

TEST_F(variant_test_fixture, fen_round_trips_all_fields) {
  for(const string fen : { "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1 -",
                           "4k3/2m5/8/8/8/2P1P3/8/1A2K3 w - - 7 31 Vv c3",
                           "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQ - 0 12 v -" }) {
    board.parse_fen(fen);
    istringstream fields(fen);
    string skipped;
    int fullmove{ };
    fields >> skipped >> skipped >> skipped >> skipped >> skipped >> fullmove;
    EXPECT_EQ(board.to_fen(fullmove), fen);
  }

  // a legacy six-field FEN gains the variant field
  board.parse_fen(Fen::start_position);
  EXPECT_EQ(board.to_fen(1), string(Fen::start_position) + " -");
}

TEST_F(variant_test_fixture, incremental_hash_matches_a_fresh_hash) {
  board.parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  for(const char* move : { "e1g1", "b4c3", "d2c3", "e8c8", "a2a4", "c7c5", "d5c6" }) {
    ASSERT_TRUE(engine.apply_move(move)) << move;
    EXPECT_EQ(board.state.hash_key, generate_hash_key(board.state)) << move;
  }

  board.parse_fen("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1 V");
  ASSERT_TRUE(engine.apply_move("b7b8m"));
  EXPECT_EQ(board.state.hash_key, generate_hash_key(board.state));
}

TEST_F(variant_test_fixture, custom_positions_are_reproducible_and_valid) {
  Board other{ };
  for(unsigned int seed{ 1 }; seed <= 20; ++seed) {
    ASSERT_TRUE(generate_custom_position(board, static_cast<int>(seed % 2), seed));
    ASSERT_TRUE(generate_custom_position(other, static_cast<int>(seed % 2), seed));
    const string fen = custom_position_fen(board);
    EXPECT_EQ(fen, custom_position_fen(other));
    EXPECT_TRUE(fen.ends_with(" - - 0 1 Vv")) << fen;
    EXPECT_EQ(count_bits(board.state.bitboards[K]), 1);
    EXPECT_EQ(count_bits(board.state.bitboards[k]), 1);
    EXPECT_FALSE(board.in_check()) << fen;
    EXPECT_TRUE(board.has_legal_move()) << fen;
  }
}

// every army spends exactly its 39 points, keeps a pawn wall in front of a king that
// never castles, and holds a compound piece (both sides play variant rules)
TEST_F(variant_test_fixture, custom_armies_have_a_pawn_wall_and_a_compound_piece) {
  constexpr array<int, 8> weights{ 1, 3, 3, 5, 9, 6, 8, 13 };
  constexpr array<Pieces, 8> white_pieces{ P, N, B, R, Q, A, C, M };
  constexpr array<Pieces, 8> black_pieces{ p, n, b, r, q, a, c, m };

  for(unsigned int seed{ 1 }; seed <= 200; ++seed) {
    ASSERT_TRUE(generate_custom_position(board, white, seed));
    const string fen = custom_position_fen(board);

    for(const auto& pieces : { white_pieces, black_pieces }) {
      int army_weight{ };
      for(size_t index{ }; index < pieces.size(); ++index)
        army_weight += count_bits(board.state.bitboards[pieces[index]]) * weights[index];

      EXPECT_EQ(army_weight, 39) << fen;
      EXPECT_GE(count_bits(board.state.bitboards[pieces[0]]), 4) << fen;
      EXPECT_GE(count_bits(board.state.bitboards[pieces[5]] | board.state.bitboards[pieces[6]] | board.state.bitboards[pieces[7]]), 1) << fen;
    }
  }
}

class ffi_test_fixture : public testing::Test {
  protected:
  void SetUp() override {
    ASSERT_EQ(mah_init(), 1);
  }

  void TearDown() override {
    mah_shutdown();
  }
};

TEST_F(ffi_test_fixture, plays_moves_and_reports_the_fen) {
  char fen[160]{ };
  EXPECT_EQ(mah_apply_move("e2e4"), 1);
  EXPECT_EQ(mah_apply_move("e2e4"), 0);
  ASSERT_EQ(mah_get_fen(fen, sizeof(fen), 1), 1);
  EXPECT_STREQ(fen, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1 -");
  EXPECT_EQ(mah_get_fen(fen, 10, 1), 0);
}

TEST_F(ffi_test_fixture, rejects_positions_without_both_kings) {
  EXPECT_EQ(mah_set_position_fen("8/8/8/8/8/8/8/4K3 w - - 0 1"), 0);
  EXPECT_EQ(mah_set_position_fen("not a fen"), 0);
  EXPECT_EQ(mah_set_position_fen(nullptr), 0);
  // the start position is left behind
  char fen[160]{ };
  ASSERT_EQ(mah_get_fen(fen, sizeof(fen), 1), 1);
  EXPECT_EQ(string(fen), string(Fen::start_position) + " -");
}

TEST_F(ffi_test_fixture, classifies_terminal_positions) {
  ASSERT_EQ(mah_set_position_fen("R5k1/5ppp/8/8/8/8/8/4K3 b - - 0 1"), 1);
  EXPECT_EQ(mah_game_status(), mah_status_checkmate);
  ASSERT_EQ(mah_set_position_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"), 1);
  EXPECT_EQ(mah_game_status(), mah_status_stalemate);
  ASSERT_EQ(mah_set_position_startpos(), 1);
  EXPECT_EQ(mah_game_status(), mah_status_ongoing);
}

TEST_F(ffi_test_fixture, finds_best_moves) {
  char move[8]{ };
  ASSERT_EQ(mah_set_position_fen("6k1/5ppp/8/8/8/8/8/R3K3 w - - 0 1"), 1);
  ASSERT_EQ(mah_best_move_depth(3, move, sizeof(move)), 1);
  EXPECT_STREQ(move, "a1a8");
  ASSERT_EQ(mah_best_move_time(50, move, sizeof(move)), 1);
  EXPECT_STREQ(move, "a1a8");
  EXPECT_EQ(mah_best_move_depth(3, move, 4), 0);
}

TEST_F(ffi_test_fixture, rules_profile_pins_variant_rules) {
  char move[8]{ };
  ASSERT_EQ(mah_set_position_fen_with_rules("k7/2P5/1K6/8/8/8/8/8 w - - 0 1", mah_rules_variant), 1);
  EXPECT_EQ(mah_apply_move("c7c8m"), 1);
  ASSERT_EQ(mah_set_position_fen_with_rules("k7/2P5/1K6/8/8/8/8/8 w - - 0 1", mah_rules_standard), 1);
  EXPECT_EQ(mah_apply_move("c7c8m"), 0);
  EXPECT_EQ(mah_best_move_depth(2, move, sizeof(move)), 1);
}

TEST_F(ffi_test_fixture, generates_custom_positions) {
  char first[128]{ }, second[128]{ };
  ASSERT_EQ(mah_generate_custom_position_fen(0, 42, first, sizeof(first)), 1);
  ASSERT_EQ(mah_generate_custom_position_fen(0, 42, second, sizeof(second)), 1);
  EXPECT_STREQ(first, second);
  EXPECT_EQ(mah_set_position_fen(first), 1);
  EXPECT_EQ(mah_game_status(), mah_status_ongoing);
}

TEST_F(ffi_test_fixture, reports_strength_and_eval_status) {
  char status[64]{ }, info[512]{ };
  ASSERT_EQ(mah_get_eval_status(status, sizeof(status)), 1);
  EXPECT_STREQ(status, "classic_ready");

  ASSERT_EQ(mah_set_difficulty_level(2), 1);
  ASSERT_EQ(mah_set_eval_mode(1), 1);
  const unsigned char weights[]{ 1, 2, 3 };
  ASSERT_EQ(mah_load_weights_from_bytes(weights, 3, "test-net"), 1);
  ASSERT_EQ(mah_get_eval_status(status, sizeof(status)), 1);
  EXPECT_STREQ(status, "nnue_stub_fallback");

  ASSERT_EQ(mah_get_nnue_info(info, sizeof(info)), 1);
  const string json{ info };
  EXPECT_NE(json.find("\"requestedEvalMode\":\"nnue\""), string::npos);
  EXPECT_NE(json.find("\"activeEvalMode\":\"classic\""), string::npos);
  EXPECT_NE(json.find("\"weightsVersion\":\"test-net\""), string::npos);
  EXPECT_NE(json.find("\"uiDifficulty\":2"), string::npos);
  EXPECT_NE(json.find("\"skillLevel\":4"), string::npos);

  EXPECT_EQ(mah_load_weights("/nonexistent/weights.bin"), 0);
  EXPECT_EQ(mah_unload_weights(), 1);
}
