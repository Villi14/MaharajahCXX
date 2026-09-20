#include "../headers/Constants.h"
#include "../headers/Game.h"
#include "../headers/Perft.h"
#include "TestAccess.h"
#include "gtest/gtest.h"

#include <sstream>

using namespace std;
using namespace maharajah;

namespace {

struct PerftCase {
  const char* name;
  const char* fen;
  int depth;
  u64 nodes;
};

// https://www.chessprogramming.org/Perft_Results; every other position was counted with Stockfish (go perft N).
const PerftCase perft_cases[] = {
  { "start_d5", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609 },
  { "kiwipete_d4", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603 },
  { "position3_d5", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },
  { "position4_d4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333 },
  { "position5_d3", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379 },
  { "position6_d3", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890 },
  { "en_passant_pin_d6", "8/5bk1/8/2Pp4/8/1K6/8/8 w - d6 0 1", 6, 824064 },
  { "en_passant_black_d6b", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467 },
  { "castle_kingside_only_d6", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072 },
  { "castle_queenside_only_d6", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711 },
  { "castle_through_check_d4", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206 },
  { "castle_black_d4", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476 },
  { "promotion_d4", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 4, 182838 },
  { "en_passant_check_d5", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 5, 135655 },
  { "en_passant_black_d6", "8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1", 6, 2822114 },
  { "castle_rook_only_d6", "8/8/8/8/8/8/6k1/4K2R w K - 0 1", 6, 185867 },
  { "queen_knight_d6", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 6, 3114998 },
  { "rook_endgame_castle_white_d4", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 4, 314346 },
  { "rook_endgame_castle_black_d4", "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 4, 314346 },
  { "ep_discovered_check_black_d6", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888 },
  { "position4_mirrored_d4", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333 },
  { "promotion_and_capture_d4", "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1", 4, 1032012 },
  { "cmk_position_d4", "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9", 4, 1679340 },
  { "ep_gives_check_d5", "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3", 5, 36744 },
  { "knight_edge_start_d4", "r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2", 4, 182327 },
  { "castle_and_pins_d4", "r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R w KQkq - 0 1", 4, 150072 },
  { "pawn_endgame_d5", "8/5p2/8/2k3P1/p3K3/8/1P6/8 b - - 0 1", 5, 72120 },
  { "rook_capture_removes_castle_d5", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 5, 1063513 },
  { "castle_kingside_lone_king_d6", "3k4/8/8/8/8/8/8/4K2R w K - 0 1", 6, 820709 },
  { "castle_only_black_queenside_d6", "r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 6, 846648 },
  { "kings_only_d6", "4k3/8/8/8/8/8/8/4K3 w - - 0 1", 6, 53896 },
  { "stalemate_position_d4", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", 4, 0 },
  { "mate_position_d4", "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", 4, 0 },
  { "pinned_knight_d4", "rn1qkbnr/ppp1pppp/8/3p4/6b1/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1", 4, 476585 },
  { "queen_vs_pawns_d5", "8/8/8/8/8/2k5/1p6/1K1Q4 w - - 0 1", 5, 154550 },
  { "double_check_d5", "4k3/8/8/8/1b6/8/2n5/R3K3 w Q - 0 1", 5, 293603 },
  { "underpromotion_race_d5", "8/PPP4k/8/8/8/8/4Kppp/8 w - - 0 1", 5, 1745545 },
};

class perft_test_fixture : public testing::TestWithParam<PerftCase> {
  public:
  Game game{ };
};

} // namespace

TEST_P(perft_test_fixture, matches_reference_nodes) {
  const auto& c = GetParam();
  game.parse_fen(c.fen);
  Perft perft(GameTestAccess::board(game));

  EXPECT_EQ(perft.perft(c.depth), c.nodes);
}

TEST_P(perft_test_fixture, leaves_board_unchanged) {
  const auto& c = GetParam();
  game.parse_fen(c.fen);
  auto& board = GameTestAccess::board(game);
  const auto before = board.state;

  Perft perft(board);
  (void)perft.perft(min(c.depth, 3));

  EXPECT_EQ(board.ply, 0);
  EXPECT_EQ(board.state.side, before.side);
  EXPECT_EQ(board.state.en_passant, before.en_passant);
  EXPECT_EQ(board.state.castle, before.castle);
  EXPECT_EQ(board.state.bitboards, before.bitboards);
  EXPECT_EQ(board.state.occupancies, before.occupancies);
}

INSTANTIATE_TEST_SUITE_P(reference_positions, perft_test_fixture, testing::ValuesIn(perft_cases), [](const testing::TestParamInfo<PerftCase>& info) {
  return string(info.param.name);
});

class perft_edge_fixture : public testing::Test {
  public:
  Game game{ };
};

TEST_F(perft_edge_fixture, depth_zero_is_one_node) {
  game.parse_fen(Fen::start_position);
  Perft perft(GameTestAccess::board(game));
  EXPECT_EQ(perft.perft(0), 1U);
}

TEST_F(perft_edge_fixture, divide_and_test_with_zero_depth_do_not_recurse) {
  game.parse_fen(Fen::start_position);
  auto& board = GameTestAccess::board(game);
  Perft perft(board);

  testing::internal::CaptureStdout();
  perft.perft_divide(0);
  perft.perft_test(0);
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(board.ply, 0);
}

TEST_F(perft_edge_fixture, divide_sums_to_perft) {
  game.parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  Perft perft(GameTestAccess::board(game));

  testing::internal::CaptureStdout();
  perft.perft_divide(3);
  const string out = testing::internal::GetCapturedStdout();

  u64 sum = 0;
  istringstream lines(out);
  string line;
  while(getline(lines, line)) {
    const auto pos = line.find(": ");
    ASSERT_NE(pos, string::npos);
    sum += stoull(line.substr(pos + 2));
  }

  EXPECT_EQ(sum, 97862U);
}
