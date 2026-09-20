#include "../headers/Bitboard.h"
#include "../headers/Constants.h"
#include "../headers/Evaluator.h"
#include "../headers/Game.h"
#include "../headers/Search.h"
#include "TestAccess.h"
#include "gtest/gtest.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace maharajah;

namespace {

// Feeds `input` to Game::uci_loop and returns everything it wrote to stdout.
string run_uci(Game& game, const string& input) {
  istringstream in(input);
  ostringstream out;
  auto* old_in = cin.rdbuf(in.rdbuf());
  auto* old_out = cout.rdbuf(out.rdbuf());

  game.uci_loop();

  cin.rdbuf(old_in);
  cout.rdbuf(old_out);
  return out.str();
}

} // namespace

class search_test_fixture : public testing::Test {
  public:
  Game game{ };
  Board& board() {
    return GameTestAccess::board(game);
  }

  void play(const char* move) {
    const int parsed = game.parse_move(move);
    ASSERT_NE(parsed, 0) << move;
    ASSERT_TRUE(board().make_move(parsed, TypeMove::all_moves)) << move;
  }
};

TEST_F(search_test_fixture, uci_startpos_returns_bestmove) {
  const string out = run_uci(game, "position startpos\ngo depth 3\nquit\n");
  EXPECT_NE(out.find("bestmove "), string::npos);
}

TEST_F(search_test_fixture, uci_bestmove_after_moves_is_returned) {
  // The root is detected by search ply, not by the number of moves already made.
  const string out = run_uci(game, "position startpos moves e2e4 e7e5\ngo depth 3\nquit\n");
  EXPECT_NE(out.find("bestmove "), string::npos);
}

TEST_F(search_test_fixture, uci_does_not_print_board_or_debug_noise) {
  const string out = run_uci(game, "position startpos\ngo depth 2\nquit\n");
  EXPECT_EQ(out.find("a b c d e f g h"), string::npos);
  EXPECT_EQ(out.find("depth: "), string::npos);
}

TEST_F(search_test_fixture, uci_illegal_move_is_reported_and_stops_parsing) {
  const string out = run_uci(game, "position startpos moves e2e4 e1e2\nquit\n");
  EXPECT_EQ(board().ply, 1);
}

TEST_F(search_test_fixture, uci_rejects_position_without_kings) {
  const string out = run_uci(game, "position fen 8/8/8/8/8/8/8/8 w - - 0 1\nquit\n");
  EXPECT_NE(out.find("invalid position"), string::npos);
  EXPECT_EQ(count_bits(board().state.bitboards[K]), 1);
}

TEST_F(search_test_fixture, uci_rejects_malformed_fen) {
  const string out = run_uci(game, "position fen nonsense\nquit\n");
  EXPECT_NE(out.find("invalid fen"), string::npos);
}

TEST_F(search_test_fixture, finds_mate_in_one) {
  game.parse_fen("6k1/5ppp/8/8/8/8/8/R3K3 w - - 0 1");
  const SearchResult result = Search(board()).run(3);
  EXPECT_EQ(result.best_move, game.parse_move("a1a8"));
  EXPECT_GT(result.score, Search::mate_score - 10);
}

TEST_F(search_test_fixture, checkmated_side_gets_mate_score) {
  game.parse_fen("R5k1/5ppp/8/8/8/8/8/4K3 b - - 0 1");
  EXPECT_EQ(Search(board()).negamax(-Search::infinity, Search::infinity, 1), -Search::mate_score);
}

TEST_F(search_test_fixture, stalemate_scores_zero) {
  game.parse_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  EXPECT_EQ(Search(board()).negamax(-Search::infinity, Search::infinity, 1), 0);
}

TEST_F(search_test_fixture, start_position_evaluates_to_zero) {
  game.parse_fen(Fen::start_position);
  EXPECT_EQ(Evaluator::evaluate(board().state), 0);
}

TEST_F(search_test_fixture, evaluation_is_symmetric_for_mirrored_positions) {
  game.parse_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
  const int white_view = Evaluator::evaluate(board().state);
  game.parse_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  EXPECT_EQ(Evaluator::evaluate(board().state), -white_view);
}

TEST_F(search_test_fixture, halfmove_clock_is_parsed_from_fen) {
  game.parse_fen("4k3/8/8/8/8/8/8/K6R w - - 37 60");
  EXPECT_EQ(board().state.halfmove, 37);
  game.parse_fen(Fen::start_position);
  EXPECT_EQ(board().state.halfmove, 0);
}

TEST_F(search_test_fixture, halfmove_clock_resets_on_pawn_move_and_capture) {
  game.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  play("g1f3");
  EXPECT_EQ(board().state.halfmove, 1);
  play("g8f6");
  EXPECT_EQ(board().state.halfmove, 2);
  play("e2e4");
  EXPECT_EQ(board().state.halfmove, 0);
  play("f6e4");
  EXPECT_EQ(board().state.halfmove, 0);
}

TEST_F(search_test_fixture, fifty_move_rule_is_a_draw) {
  game.parse_fen("4k3/8/8/8/8/8/8/K6R w - - 99 80");
  EXPECT_FALSE(Search(board()).is_draw());
  game.parse_fen("4k3/8/8/8/8/8/8/K6R w - - 100 80");
  EXPECT_TRUE(Search(board()).is_draw());
}

TEST_F(search_test_fixture, repetition_is_a_draw) {
  game.parse_fen("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
  EXPECT_FALSE(Search(board()).is_draw());
  play("h1h2");
  play("e8d8");
  play("h2h1");
  EXPECT_FALSE(Search(board()).is_draw());
  play("d8e8");
  EXPECT_TRUE(Search(board()).is_draw());
}

TEST_F(search_test_fixture, repetition_ignores_positions_before_irreversible_move) {
  game.parse_fen("4k3/8/8/8/8/8/P7/4K2R w - - 0 1");
  play("h1h2");
  play("e8d8");
  play("a2a3"); // pawn move, cannot be undone
  play("d8e8");
  play("h2h1");
  play("e8d8");
  EXPECT_FALSE(Search(board()).is_draw());
}

TEST_F(search_test_fixture, run_reports_depth_and_nodes) {
  game.parse_fen(Fen::start_position);
  const SearchResult result = Search(board()).run(3);
  EXPECT_EQ(result.depth, 3);
  EXPECT_GT(result.nodes, 0);
  EXPECT_NE(result.best_move, 0);
}

TEST_F(search_test_fixture, run_has_no_best_move_when_mated_or_stalemated) {
  game.parse_fen("R5k1/5ppp/8/8/8/8/8/4K3 b - - 0 1");
  EXPECT_EQ(Search(board()).run(2).best_move, 0);
  game.parse_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  EXPECT_EQ(Search(board()).run(2).best_move, 0);
}

TEST_F(search_test_fixture, run_leaves_the_board_unchanged) {
  game.parse_fen(Fen::start_position);
  const auto before = board().state;
  (void)Search(board()).run(3);
  EXPECT_EQ(board().ply, 0);
  EXPECT_EQ(board().state.bitboards, before.bitboards);
  EXPECT_EQ(board().state.side, before.side);
}

TEST_F(search_test_fixture, repeated_runs_are_deterministic) {
  game.parse_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
  const SearchResult first = Search(board()).run(3);
  const SearchResult second = Search(board()).run(3);
  EXPECT_EQ(first.best_move, second.best_move);
  EXPECT_EQ(first.nodes, second.nodes);
}
