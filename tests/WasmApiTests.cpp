// The browser adapters (mah_wasm_*), built natively: they must return what the
// buffer-filling C interface (mah_*) writes.

#include "../headers/ffi/maharajah_ffi.h"
#include "../headers/wasm/maharajah_wasm.h"
#include "gtest/gtest.h"

#include <array>
#include <string>

using namespace std;

namespace {

constexpr auto start_fen{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };

class WasmApi : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_EQ(mah_wasm_init(), 1);
  }
};

TEST_F(WasmApi, GetFenMatchesFfi) {
  ASSERT_EQ(mah_wasm_set_position_fen(start_fen), 1);
  ASSERT_EQ(mah_wasm_apply_move("e2e4"), 1);

  array<char, 256> expected{ };
  ASSERT_EQ(mah_get_fen(expected.data(), static_cast<int>(expected.size()), 1), 1);
  EXPECT_EQ(string{ mah_wasm_get_fen(1) }, string{ expected.data() });
  EXPECT_EQ(string{ mah_wasm_get_fen(1) }.substr(0, 20), "rnbqkbnr/pppppppp/8/");
}

TEST_F(WasmApi, RejectsBadInput) {
  EXPECT_EQ(mah_wasm_set_position_fen("8/8/8/8/8/8/8/8 w - - 0 1"), 0);
  EXPECT_EQ(mah_wasm_set_position_fen("not a fen"), 0);
  EXPECT_EQ(mah_wasm_apply_move("e2e5"), 0);
  EXPECT_EQ(mah_wasm_apply_move(""), 0);
}

TEST_F(WasmApi, BestMoveDepthMatchesFfi) {
  const char* fens[]{
    start_fen,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
  };
  for(const char* fen : fens) {
    ASSERT_EQ(mah_set_position_fen(fen), 1);
    array<char, 8> expected{ };
    ASSERT_EQ(mah_best_move_depth(4, expected.data(), static_cast<int>(expected.size())), 1);

    ASSERT_EQ(mah_wasm_set_position_fen(fen), 1);
    EXPECT_EQ(string{ mah_wasm_best_move_depth(4) }, string{ expected.data() }) << fen;
  }
}

TEST_F(WasmApi, BestMoveTimeReturnsLegalMove) {
  ASSERT_EQ(mah_wasm_set_position_fen(start_fen), 1);
  const string move{ mah_wasm_best_move_time(50) };
  ASSERT_GE(move.size(), 4u);
  EXPECT_EQ(mah_wasm_apply_move(move.c_str()), 1);
}

TEST_F(WasmApi, GameStatusAndNoMove) {
  // fool's mate: white is checkmated
  ASSERT_EQ(mah_wasm_set_position_fen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"), 1);
  EXPECT_EQ(mah_wasm_game_status(), mah_status_checkmate);
  EXPECT_EQ(string{ mah_wasm_best_move_depth(3) }, "");

  ASSERT_EQ(mah_wasm_set_position_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"), 1);
  EXPECT_EQ(mah_wasm_game_status(), mah_status_stalemate);

  ASSERT_EQ(mah_wasm_set_position_fen(start_fen), 1);
  EXPECT_EQ(mah_wasm_game_status(), mah_status_ongoing);
}

} // namespace
