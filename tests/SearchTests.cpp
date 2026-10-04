#include "../headers/Bitboard.h"
#include "../headers/Constants.h"
#include "../headers/Evaluator.h"
#include "../headers/Game.h"
#include "../headers/Search.h"
#include "../headers/See.h"
#include "TestAccess.h"
#include "gtest/gtest.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace maharajah;

namespace {

// Colour-mirrored FEN: ranks reversed, piece colours and side to move swapped.
string mirror_fen(const string& fen) {
  istringstream fields(fen);
  string placement, side, castle, en_passant, rest;
  fields >> placement >> side >> castle >> en_passant;
  getline(fields, rest);

  string mirrored;
  for(size_t end = placement.size(); end != string::npos;) {
    const size_t start = placement.rfind('/', end - 1);
    const size_t from = start == string::npos ? 0 : start + 1;
    if(!mirrored.empty())
      mirrored += '/';
    mirrored += placement.substr(from, end - from);
    end = start;
  }

  const auto swap_case = [](string text) {
    for(char& ch : text)
      ch = isupper(ch) ? static_cast<char>(tolower(ch)) : static_cast<char>(toupper(ch));
    return text;
  };

  if(en_passant != "-")
    en_passant[1] = static_cast<char>('9' - (en_passant[1] - '0'));

  return swap_case(mirrored) + (side == "w" ? " b " : " w ") + (castle == "-" ? castle : swap_case(castle)) + ' ' + en_passant + rest;
}

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
  Engine& engine() {
    return GameTestAccess::engine(game);
  }
  Board& board() {
    return GameTestAccess::board(game);
  }

  void play(const char* move) {
    ASSERT_TRUE(engine().apply_move(move)) << move;
  }

  SearchResult run(const int depth) {
    return Search(engine()).run(depth);
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
  EXPECT_NE(out.find("illegal move ignored"), string::npos);
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
  const SearchResult result = run(3);
  EXPECT_EQ(result.best_move, game.parse_move("a1a8"));
  EXPECT_GT(result.score, Search::mate_score);
}

TEST_F(search_test_fixture, checkmated_side_gets_mate_score) {
  game.parse_fen("R5k1/5ppp/8/8/8/8/8/4K3 b - - 0 1");
  EXPECT_EQ(Search(engine()).negamax(-Search::infinity, Search::infinity, 1), -Search::mate_value);
}

TEST_F(search_test_fixture, stalemate_scores_zero) {
  game.parse_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  EXPECT_EQ(Search(engine()).negamax(-Search::infinity, Search::infinity, 1), 0);
}

TEST_F(search_test_fixture, start_position_evaluates_to_the_tempo_bonus) {
  game.parse_fen(Fen::start_position);
  EXPECT_EQ(Evaluator::evaluate(board().state), Evaluation::tempo_bonus);
}

TEST_F(search_test_fixture, evaluation_is_symmetric_for_mirrored_positions) {
  for(const string fen : { "rnbqkbnr/pppp1ppp/8/4p3/3PP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 2",
                           "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                           "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                           "rnacmbnr/pppppppp/8/8/8/8/PPPPPPPP/RNACMBNR w - - 0 1" }) {
    game.parse_fen(fen);
    const int original = Evaluator::evaluate(board().state);
    game.parse_fen(mirror_fen(fen));
    EXPECT_EQ(Evaluator::evaluate(board().state), original) << fen;
  }
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
  EXPECT_FALSE(Search(engine()).is_draw());
  game.parse_fen("4k3/8/8/8/8/8/8/K6R w - - 100 80");
  EXPECT_TRUE(Search(engine()).is_draw());
}

TEST_F(search_test_fixture, fifty_move_rule_does_not_apply_to_variant_boards) {
  game.parse_fen("4k3/8/8/8/8/8/8/K6A w - - 100 80");
  EXPECT_FALSE(Search(engine()).is_draw());
}

TEST_F(search_test_fixture, threefold_repetition_is_a_draw) {
  game.parse_fen("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
  EXPECT_FALSE(Search(engine()).is_draw());
  for(int cycle{ }; cycle < 2; ++cycle) {
    play("h1h2");
    play("e8d8");
    play("h2h1");
    play("d8e8");
    // the start position has now occurred cycle + 2 times
    EXPECT_EQ(board().repetition_count(), cycle + 2);
    EXPECT_EQ(Search(engine()).is_draw(), cycle == 1);
  }
}

TEST_F(search_test_fixture, repetition_ignores_positions_before_irreversible_move) {
  game.parse_fen("4k3/8/8/8/8/8/P7/4K2R w - - 0 1");
  play("h1h2");
  play("e8d8");
  play("a2a3"); // pawn move, cannot be undone
  play("d8e8");
  play("h2h1");
  play("e8d8");
  EXPECT_EQ(board().repetition_count(), 1);
}

TEST_F(search_test_fixture, insufficient_material_is_a_draw) {
  for(const char* fen :
      { "4k3/8/8/8/8/8/8/4K3 w - - 0 1", "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", "4k3/8/8/8/8/8/8/4KN2 w - - 0 1", "2b1k3/8/8/8/8/8/8/4KB2 w - - 0 1" }) {
    game.parse_fen(fen);
    EXPECT_TRUE(Search(engine()).is_insufficient_material()) << fen;
  }

  for(const char* fen :
      { "4k3/8/8/8/8/8/8/3NKN2 w - - 0 1", "4kn2/8/8/8/8/8/8/4KN2 w - - 0 1", "3bk3/8/8/8/8/8/8/4KB2 w - - 0 1", "4k3/8/8/8/8/8/8/4KA2 w - - 0 1" }) {
    game.parse_fen(fen);
    EXPECT_FALSE(Search(engine()).is_insufficient_material()) << fen;
  }
}

TEST_F(search_test_fixture, run_reports_depth_and_nodes) {
  game.parse_fen(Fen::start_position);
  const SearchResult result = run(3);
  EXPECT_EQ(result.depth, 3);
  EXPECT_GT(result.nodes, 0);
  EXPECT_NE(result.best_move, 0);
}

TEST_F(search_test_fixture, run_has_no_best_move_when_mated_or_stalemated) {
  game.parse_fen("R5k1/5ppp/8/8/8/8/8/4K3 b - - 0 1");
  EXPECT_EQ(run(2).best_move, 0);
  game.parse_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  EXPECT_EQ(run(2).best_move, 0);
}

TEST_F(search_test_fixture, run_leaves_the_board_unchanged) {
  game.parse_fen(Fen::start_position);
  const auto before = board().state;
  (void)run(4);
  EXPECT_EQ(board().ply, 0);
  EXPECT_EQ(board().repetition_index, 0);
  EXPECT_EQ(board().state.bitboards, before.bitboards);
  EXPECT_EQ(board().state.side, before.side);
  EXPECT_EQ(board().state.hash_key, before.hash_key);
}

TEST_F(search_test_fixture, repeated_runs_with_a_cleared_hash_table_are_deterministic) {
  game.parse_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
  const SearchResult first = run(5);
  engine().transposition_table.clear();
  const SearchResult second = run(5);
  EXPECT_EQ(first.best_move, second.best_move);
  EXPECT_EQ(first.nodes, second.nodes);
}

TEST_F(search_test_fixture, low_skill_caps_the_search_depth) {
  game.parse_fen(Fen::start_position);
  engine().search_config = SearchConfig::for_difficulty(1);
  EXPECT_EQ(run(8).depth, 1);
  engine().search_config = SearchConfig::for_skill(7);
  EXPECT_EQ(run(8).depth, 5);
}

TEST_F(search_test_fixture, low_skill_still_takes_a_free_queen) {
  engine().search_config = SearchConfig::for_difficulty(1);
  for(unsigned int attempt{ }; attempt < 20; ++attempt) {
    game.parse_fen("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
    EXPECT_EQ(Board::move_to_string(run(4).best_move), "d1d5");
  }
}

TEST_F(search_test_fixture, compound_promotions_follow_the_side_rules) {
  game.parse_fen("k7/2P5/1K6/8/8/8/8/8 w - - 0 1 V");
  EXPECT_NE(game.parse_move("c7c8m"), 0);
  EXPECT_NE(game.parse_move("c7c8a"), 0);
  game.parse_fen("k7/2P5/1K6/8/8/8/8/8 w - - 0 1 v");
  EXPECT_EQ(game.parse_move("c7c8m"), 0);
  EXPECT_NE(game.parse_move("c7c8q"), 0);
}

TEST_F(search_test_fixture, finds_an_amazon_mate) {
  // smothered mate with the amazon's knight move; a queen on c4 has no mate
  game.parse_fen("6rk/6pp/8/8/2M5/8/8/K7 w - - 0 1");
  const SearchResult result = run(2);
  EXPECT_EQ(Board::move_to_string(result.best_move), "c4f7");
  EXPECT_GT(result.score, Search::mate_score);
  game.parse_fen("6rk/6pp/8/8/2Q5/8/8/K7 w - - 0 1");
  EXPECT_LT(run(2).score, Search::mate_score);
}

TEST_F(search_test_fixture, see_values_exchanges) {
  // pawn takes a defended knight: wins a knight for a pawn
  game.parse_fen("4k3/8/2p5/3n4/4P3/8/8/4K3 w - - 0 1");
  EXPECT_EQ(see_evaluate(board(), game.parse_move("e4d5")), 337 - 82);
  // queen takes a pawn defended by a pawn: loses the queen for a pawn
  game.parse_fen("4k3/8/2p5/3p4/8/8/3Q4/4K3 w - - 0 1");
  EXPECT_EQ(see_evaluate(board(), game.parse_move("d2d5")), 82 - 1025);
  // quiet move
  game.parse_fen(Fen::start_position);
  EXPECT_EQ(see_evaluate(board(), game.parse_move("e2e4")), 0);
}

TEST_F(search_test_fixture, uci_setoption_changes_skill_and_hash) {
  (void)run_uci(game, "setoption name Skill Level value 3\nsetoption name Hash value 16\nquit\n");
  EXPECT_EQ(engine().search_config.skill_level, 3);
  EXPECT_EQ(engine().transposition_table.entries(), 16u * 0x100000 / 24);
}

TEST_F(search_test_fixture, uci_reports_none_without_legal_moves) {
  const string out = run_uci(game, "position fen k7/2Q5/1K6/8/8/8/8/8 b - - 0 1\ngo depth 1\nquit\n");
  EXPECT_NE(out.find("bestmove (none)"), string::npos);
}

TEST_F(search_test_fixture, uci_info_lines_carry_score_depth_and_pv) {
  const string out = run_uci(game, "position fen 6k1/5ppp/8/8/8/8/8/R3K3 w - - 0 1\ngo depth 3\nquit\n");
  // a mate in one needs a two-ply search to see that the reply has no moves
  EXPECT_NE(out.find("info score mate 1 depth 2 "), string::npos);
  EXPECT_NE(out.find("pv a1a8"), string::npos);
  EXPECT_NE(out.find("bestmove a1a8"), string::npos);
}

TEST_F(search_test_fixture, uci_position_fen_with_moves_keeps_the_fen_fields) {
  // the moves list must not be read as the optional variant / unmoved-pawn FEN fields
  (void)run_uci(game, "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4 e7e5\nquit\n");
  EXPECT_EQ(board().state.pawn_unmoved, zero);
  EXPECT_EQ(board().repetition_index, 2);
  EXPECT_NE(game.parse_move("d2d4"), 0);
}

TEST_F(search_test_fixture, uci_keeps_the_hash_table_between_moves_until_ucinewgame) {
  Board start;
  start.parse_fen(Fen::start_position);
  const auto stored_move = [&] {
    return engine().transposition_table.probe(start.state.hash_key, -Search::infinity, Search::infinity, 0, 0).move;
  };

  (void)run_uci(game, "position startpos\ngo depth 4\nposition startpos moves e2e4\nquit\n");
  EXPECT_NE(stored_move(), 0);

  (void)run_uci(game, "ucinewgame\nquit\n");
  EXPECT_EQ(stored_move(), 0);
}

TEST_F(search_test_fixture, uci_clock_sets_soft_and_hard_limits) {
  // share 3000 / 30 = 100 ms: no new iteration after 60 ms, a running one stops at 250 ms
  (void)run_uci(game, "position startpos\ngo wtime 3000 btime 3000");
  const TimeControl& time = engine().time_control;
  EXPECT_EQ(time.soft_stoptime - time.starttime, 60);
  EXPECT_EQ(time.stoptime - time.starttime, 250);

  // share 1000 / 30 + 200 = 233 ms; 2.5 times that would pass 40 % of the time left
  (void)run_uci(game, "position startpos\ngo wtime 1000 btime 1000 winc 200 binc 200");
  EXPECT_EQ(time.soft_stoptime - time.starttime, 139);
  EXPECT_EQ(time.stoptime - time.starttime, 400);

  // a fixed move time has no soft limit
  (void)run_uci(game, "position startpos\ngo movetime 30");
  EXPECT_EQ(time.soft_stoptime, 0);
  EXPECT_EQ(time.stoptime - time.starttime, 30);
}

TEST_F(search_test_fixture, uci_go_movetime_returns_a_move) {
  // input is exhausted when the search starts, so it runs until the time is up
  const string out = run_uci(game, "position startpos\ngo movetime 50");
  EXPECT_NE(out.find("bestmove "), string::npos);
  EXPECT_EQ(out.find("bestmove (none)"), string::npos);
}

// Lazy SMP (run_search with more than one thread)

TEST_F(search_test_fixture, one_thread_run_search_is_the_plain_search) {
  game.parse_fen("r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8");
  const SearchResult plain = run(6);
  engine().transposition_table.clear();
  const SearchResult threaded = run_search(engine(), 6);
  EXPECT_EQ(threaded.best_move, plain.best_move);
  EXPECT_EQ(threaded.score, plain.score);
  EXPECT_EQ(threaded.nodes, plain.nodes);
}

TEST_F(search_test_fixture, helper_threads_leave_the_position_and_find_the_mate) {
  engine().threads = 4;
  game.parse_fen("6rk/6pp/8/8/2M5/8/8/K7 w - - 0 1");
  const BoardState before = board().state;

  const SearchResult result = run_search(engine(), 6);
  EXPECT_EQ(Board::move_to_string(result.best_move), "c4f7");
  EXPECT_GT(result.score, Search::mate_score);
  EXPECT_EQ(result.depth, 6);
  EXPECT_EQ(board().state.hash_key, before.hash_key);
  EXPECT_EQ(board().state.bitboards, before.bitboards);
}

TEST_F(search_test_fixture, helper_threads_stop_with_the_main_search_on_time) {
  engine().threads = 4;
  game.parse_fen(Fen::start_position);
  engine().time_control.set_movetime(200);
  const SearchResult result = run_search(engine(), Limits::max_ply);
  EXPECT_NE(result.best_move, 0);
  EXPECT_LT(result.depth, Limits::max_ply);
}

TEST_F(search_test_fixture, uci_threads_option_is_clamped_and_searches) {
  const string out = run_uci(game, "setoption name Threads value 3\nposition startpos\ngo depth 5\nquit\n");
  EXPECT_EQ(engine().threads, 3);
  EXPECT_NE(out.find("option name Threads type spin default 1 min 1 max 64"), string::npos);
  EXPECT_NE(out.find("bestmove "), string::npos);

  (void)run_uci(game, "setoption name Threads value 1000\nquit\n");
  EXPECT_EQ(engine().threads, Engine::max_threads);
}
