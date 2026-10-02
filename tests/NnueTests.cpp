// NNUE: network loading, incremental accumulator updates and the search hook-up.

#include "../headers/CustomSetup.h"
#include "../headers/Nnue.h"
#include "../headers/Search.h"
#include "TestNetwork.h"
#include "TestPositions.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

using namespace std;
using namespace maharajah;

namespace {

const vector<unsigned char>& network() {
  static const vector<unsigned char> bytes = random_network_bytes(1);
  return bytes;
}

bool accumulators_equal(const NnueAccumulator& left, const NnueAccumulator& right, const int hidden) {
  for(const Colors side : { white, black }) {
    if(!std::equal(left.values[side].begin(), left.values[side].begin() + hidden, right.values[side].begin()))
      return false;
  }
  return true;
}

// ranks mirrored, colours swapped, the other side to move
BoardState mirrored(const BoardState& state) {
  BoardState result = state;
  for(int piece{ }; piece < PieceCount::all; ++piece) {
    u64 flipped{ };
    for(int rank{ }; rank < 8; ++rank)
      flipped |= ((state.bitboards[piece] >> (8 * rank)) & 0xFF) << (8 * (7 - rank));
    result.bitboards[(piece + PieceCount::per_side) % PieceCount::all] = flipped;
  }
  result.side = opponent(state.side);
  return result;
}

} // namespace

TEST(nnue_test, loads_only_a_network_of_its_own_shape) {
  Nnue nnue;
  EXPECT_FALSE(nnue.has_loaded_weights());
  EXPECT_TRUE(Nnue::backend_ready());

  ASSERT_TRUE(nnue.load_weights_from_bytes(network().data(), network().size(), "random-1"));
  EXPECT_TRUE(nnue.has_loaded_weights());
  EXPECT_EQ(nnue.weights_version(), "random-1");
  EXPECT_EQ(nnue.loaded_path(), "<memory>");

  vector<unsigned char> bad = network();
  bad.pop_back();
  EXPECT_FALSE(nnue.load_weights_from_bytes(bad.data(), bad.size(), "short"));
  EXPECT_FALSE(nnue.has_loaded_weights());

  bad = network();
  bad[0] = 'X';
  EXPECT_FALSE(nnue.load_weights_from_bytes(bad.data(), bad.size(), "magic"));

  bad = network();
  bad[12] = 1; // hidden size 257
  EXPECT_FALSE(nnue.load_weights_from_bytes(bad.data(), bad.size(), "shape"));
  bad = random_network_bytes(1, 2048);
  EXPECT_FALSE(nnue.load_weights_from_bytes(bad.data(), bad.size(), "too wide"));
  EXPECT_EQ(nnue.hidden(), 0);
  EXPECT_FALSE(nnue.load_weights("/nonexistent/net.nnue"));
}

TEST(nnue_test, mirrored_position_evaluates_the_same_for_the_side_to_move) {
  Nnue nnue;
  ASSERT_TRUE(nnue.load_weights_from_bytes(network().data(), network().size(), "random-1"));

  for(const string_view fen : { TestFen::tricky_position, TestFen::cmk_position, string_view{ "r1a1k1cr/pppp1ppp/8/4m3/4M3/8/PPPP1PPP/R1A1K1CR w - - 0 1 Vv" } }) {
    Board board;
    board.parse_fen(fen);
    EXPECT_EQ(nnue.evaluate(board.state), nnue.evaluate(mirrored(board.state))) << fen;
  }
}

// every move kind, compound pieces and promotions to them included, for each size
class nnue_size_test : public testing::TestWithParam<int> { };

INSTANTIATE_TEST_SUITE_P(hidden_sizes, nnue_size_test, testing::Values(32, 256, 512, 1024));

TEST_P(nnue_size_test, incremental_updates_match_a_full_refresh) {
  const int hidden = GetParam();
  const vector<unsigned char> bytes = random_network_bytes(static_cast<unsigned>(hidden), hidden);
  Nnue nnue;
  ASSERT_TRUE(nnue.load_weights_from_bytes(bytes.data(), bytes.size(), "random"));
  ASSERT_EQ(nnue.hidden(), hidden);

  vector<string> fens{ string{ TestFen::tricky_position }, string{ TestFen::killer_position }, "4k3/1P4P1/8/8/8/8/1p4p1/4K3 w - - 0 1 Vv" };
  for(unsigned seed{ 1 }; seed <= 6; ++seed) {
    Board board;
    ASSERT_TRUE(generate_custom_position(board, -1, seed));
    fens.push_back(custom_position_fen(board));
  }

  mt19937 rng(7);
  int updates{ };
  for(const string& fen : fens) {
    Board board;
    board.parse_fen(fen);
    NnueAccumulator parent, updated, refreshed;
    nnue.refresh(board.state, parent);

    for(int ply{ }; ply < 60; ++ply) {
      MoveList moves;
      board.generate_moves(moves);
      vector<int> legal;
      for(size_t i{ }; i < moves.size(); ++i) {
        Board copy = board;
        if(copy.make_move(moves[i], TypeMove::all_moves))
          legal.push_back(moves[i]);
      }
      if(legal.empty())
        break;

      ASSERT_TRUE(board.make_move(legal[uniform_int_distribution<size_t>(0, legal.size() - 1)(rng)], TypeMove::all_moves));
      nnue.update(board.history[board.ply - 1], board.state, parent, updated);
      nnue.refresh(board.state, refreshed);
      ASSERT_TRUE(accumulators_equal(updated, refreshed, hidden)) << fen << " ply " << ply;
      EXPECT_EQ(nnue.evaluate(updated, board.state.side), nnue.evaluate(board.state));
      parent = updated;
      ++updates;
    }
  }
  EXPECT_GT(updates, 300);
}

TEST(nnue_test, search_evaluates_with_the_network_when_requested) {
  Engine engine;
  engine.transposition_table.resize(4);
  ASSERT_TRUE(engine.nnue.load_weights_from_bytes(network().data(), network().size(), "random-1"));
  engine.set_position(TestFen::tricky_position);

  const SearchResult classic = Search(engine).run(5);
  engine.transposition_table.clear();
  engine.eval_config.eval_mode = EvalMode::nnue;
  const SearchResult first = Search(engine).run(5);
  engine.transposition_table.clear();
  const SearchResult second = Search(engine).run(5);

  EXPECT_NE(first.best_move, 0);
  // the accumulators carry no state between searches
  EXPECT_EQ(first.nodes, second.nodes);
  EXPECT_EQ(first.score, second.score);
  EXPECT_NE(first.nodes, classic.nodes);
}
