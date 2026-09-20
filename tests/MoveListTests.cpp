#include "../headers/MoveList.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class move_list_test_fixture : public testing::Test {
  protected:
  void SetUp() override;

  public:
  MoveList moveList;
};

void move_list_test_fixture::SetUp() { }

TEST_F(move_list_test_fixture, add_increases_size_and_stores_moves) {
  EXPECT_EQ(moveList.size(), 0);

  moveList.add(42);
  moveList.add(123);

  EXPECT_EQ(moveList.size(), 2);
  EXPECT_EQ(moveList[0], 42);
  EXPECT_EQ(moveList[1], 123);
}

TEST_F(move_list_test_fixture, clear_resets_size_but_preserves_capacity) {
  moveList.add(7);
  moveList.add(9);
  EXPECT_EQ(moveList.size(), 2);

  moveList.clear();
  EXPECT_EQ(moveList.size(), 0);

  moveList.add(11);
  EXPECT_EQ(moveList.size(), 1);
  EXPECT_EQ(moveList[0], 11);
}

TEST_F(move_list_test_fixture, operator_index_read_write) {
  moveList.add(1);
  moveList.add(2);

  moveList[0] = 100;
  moveList[1] = 200;

  EXPECT_EQ(moveList[0], 100);
  EXPECT_EQ(moveList[1], 200);
}

// TEST_F(move_list_test_fixture, add_beyond_capacity_triggers_assert) {
//   for(int i = 0; i < MAX_MOVES; ++i) {
//     moveList.add(i);
//   }

// #if !defined(NDEBUG)
//   // In a debug build, the assert should fail.
//   EXPECT_DEATH_IF_SUPPORTED(moveList.add(42), "");
// #endif
// }
