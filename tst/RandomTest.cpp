#include <gtest/gtest.h>
#include <Bitboard.h>

using namespace m;

constexpr u64 random_u32_number = 0x67d33a74;
constexpr u64 random_u64_number = 0xF81941E0FC38FD7A;
u_int random_state_number = 0x6B8B4567;

class random_test_fixture : public testing::Test {
public:
	random_test_fixture() { init_all(); }
};

TEST_F(random_test_fixture, get_random_u32_number) {
	random_state = random_state_number;
	EXPECT_EQ(random_u32_number, get_random_u32_number());
}

TEST_F(random_test_fixture, get_random_u64_number) {
	EXPECT_EQ(random_u64_number, get_random_u64_number());
}