#if 0

#include "../headers/Constants.h"
#include "../headers/Game.h"
#include "TestAccess.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

namespace {

u64 perft(Board& board, int depth) {
  if(depth == 0)
    return 1;

  board.moves_list.clear();
  board.generate_moves();

  u64 nodes = 0;
  for(size_t i = 0; i < board.moves_list.size(); ++i) {
    const int move = board.moves_list[static_cast<int>(i)];

    board.copy_board();
    board.make_move(move, TypeMove::all_moves);
    nodes += perft(board, depth - 1);
    board.take_back();
  }

  return nodes;
}

} // namespace

class perft_test_fixture : public testing::Test {
  protected:
  void SetUp() override {
    // Game constructor calls init_all() for bitboard tables.
  }

  public:
  Game game{};
};

TEST_F(perft_test_fixture, start_position_perft_small_depths) {
  game.parse_fen(start_position);
  auto& board = GameTestAccess::board(game);

  const u64 depth1 = perft(board, 1);
  const u64 depth2 = perft(board, 2);
  const u64 depth3 = perft(board, 3);

  // Basic invariants: counts are positive and increase with depth.
  EXPECT_GT(depth1, 0);
  EXPECT_GT(depth2, depth1);
  EXPECT_GT(depth3, depth2);

  // For the starting position, the standard perft(1) == 20.
  EXPECT_EQ(depth1, 20);
  // https://www.chessprogramming.org/Perft_Results
  EXPECT_EQ(depth2, 400);
  EXPECT_EQ(depth3, 8902);
}

#endif