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

/**
 * @brief Tests that adding moves to the move list increases its size and stores the moves correctly.
 *
 * @details This test checks that adding two moves to the move list increases its size to 2 and stores the moves correctly.
 */
TEST_F(move_list_test_fixture, add_increases_size_and_stores_moves) {
  EXPECT_EQ(moveList.size(), 0);

  moveList.add(42);
  moveList.add(123);

  EXPECT_EQ(moveList.size(), 2);
  EXPECT_EQ(moveList[0], 42);
  EXPECT_EQ(moveList[1], 123);
}

/**
 * @brief Tests that the clear() method resets the size of the move list but preserves its capacity.
 *
 * @details This test checks that adding two moves to the move list increases its size to 2,
 * then clearing the move list resets its size to 0, and finally adding a move to the
 * cleared move list increases its size to 1.
 */
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

/**
 * @brief Tests that the operator[] can be used to read and write moves in the move list.
 *
 * @details This test checks that adding two moves to the move list and then
 * modifying them using the operator[] changes their values correctly.
 */
TEST_F(move_list_test_fixture, operator_index_read_write) {
  moveList.add(1);
  moveList.add(2);

  moveList[0] = 100;
  moveList[1] = 200;

  EXPECT_EQ(moveList[0], 100);
  EXPECT_EQ(moveList[1], 200);
}

/**
 * @brief Tests that adding a move to the move list beyond its capacity triggers an assert.
 * @details This test checks that adding moves to the move list until its capacity is reached and
 * then attempting to add another move triggers an assert.
 */
// TEST_F(move_list_test_fixture, add_beyond_capacity_triggers_assert) {
//   for(int i = 0; i < MAX_MOVES; ++i) {
//     moveList.add(i);
//   }

// #if !defined(NDEBUG)
//   // In a debug build, the assert should fail.
//   EXPECT_DEATH_IF_SUPPORTED(moveList.add(42), "");
// #endif
// }
