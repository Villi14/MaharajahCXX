#include "../headers/Move.h"
#include "gtest/gtest.h"

#include <array>

using namespace std;
using namespace maharajah;

class move_test_fixture : public testing::Test {
  protected:
  void SetUp() override;

  public:
  const array<Move, 32> moves_list{
    Move{ e2, e4, P, no_pieces, false, false, false, false },
    Move{ e2, e4, P, no_pieces, false, false, false, true },
    Move{ e7, e8, p, no_pieces, false, false, true, false },
    Move{ e1, g1, N, no_pieces, false, false, true, true },
    Move{ e8, c8, n, no_pieces, false, true, false, true },
    Move{ e5, d6, b, no_pieces, false, true, true, false },
    Move{ e8, c8, R, no_pieces, false, true, true, true },
    Move{ e3, a4, r, no_pieces, true, false, false, false },
    Move{ a8, b8, Q, no_pieces, true, false, false, true },
    Move{ b1, c1, q, no_pieces, true, false, true, false },
    Move{ c2, d8, K, no_pieces, true, false, true, true },
    Move{ d3, e7, k, no_pieces, true, true, false, false },
    Move{ e5, f6, P, no_pieces, true, true, false, true },
    Move{ f4, g5, p, no_pieces, true, true, true, false },
    Move{ g1, c4, N, no_pieces, true, true, true, true },
    Move{ e8, c1, n, Q, false, false, false, false },
    Move{ e2, e4, B, q, false, false, false, true },
    Move{ e7, a8, b, R, false, false, true, false },
    Move{ e8, b8, r, r, false, false, true, true },
    Move{ a1, c7, K, B, false, true, false, false },
    Move{ b2, d6, k, b, false, true, true, false },
    Move{ d4, f4, n, N, false, true, true, true },
    Move{ e5, g3, P, N, true, false, false, false },
    Move{ f6, a2, p, n, true, false, false, true },
    Move{ g7, b1, N, Q, true, false, true, false },
    Move{ e8, c1, n, q, true, false, true, true },
    Move{ e1, d8, R, K, true, true, false, false },
    Move{ g7, e7, r, k, true, true, false, true },
    Move{ b1, f6, Q, P, true, true, true, false },
    Move{ a4, g4, q, p, true, true, true, true },
  };
};

void move_test_fixture::SetUp() { }

TEST_F(move_test_fixture, encode_move_test) {
  for(const auto& move : moves_list) {
    const int encoded_move = Move::encode_move(move);

    EXPECT_EQ(Move::get_move_source(encoded_move), move.source);
    EXPECT_EQ(Move::get_move_target(encoded_move), move.target);
    EXPECT_EQ(Move::get_move_piece(encoded_move), move.piece);
    EXPECT_EQ(Move::get_move_promoted(encoded_move), move.promoted);
    EXPECT_EQ(Move::get_move_capture(encoded_move), move.capture);
    EXPECT_EQ(Move::get_move_double(encoded_move), move.move_double);
    EXPECT_EQ(Move::get_move_enpassant(encoded_move), move.enpassant);
    EXPECT_EQ(Move::get_move_castling(encoded_move), move.castling);
  }
}

TEST_F(move_test_fixture, decode_move_test) {
  for(const auto& move : moves_list) {
    const int encoded_move = Move::encode_move(move);
    const Move decoded_move = Move::decode_move(encoded_move);

    EXPECT_EQ(decoded_move.source, move.source);
    EXPECT_EQ(decoded_move.target, move.target);
    EXPECT_EQ(decoded_move.piece, move.piece);
    EXPECT_EQ(decoded_move.promoted, move.promoted);
    EXPECT_EQ(decoded_move.capture, move.capture);
    EXPECT_EQ(decoded_move.move_double, move.move_double);
    EXPECT_EQ(decoded_move.enpassant, move.enpassant);
    EXPECT_EQ(decoded_move.castling, move.castling);
  }
}

TEST_F(move_test_fixture, encode_bit_layout_test) {
  // Use zeros for all fields to establish a clean baseline.
  constexpr Move base{ a8, a8, P, P, false, false, false, false };
  EXPECT_EQ(Move::encode_move(base), 0);

  constexpr Move source_max{ h1, a8, P, P, false, false, false, false };
  EXPECT_EQ(Move::encode_move(source_max), 0x3f);

  constexpr Move target_max{ a8, h1, P, P, false, false, false, false };
  EXPECT_EQ(Move::encode_move(target_max), 0xfc0);

  constexpr Move piece_max{ a8, a8, k, P, false, false, false, false };
  EXPECT_EQ(Move::encode_move(piece_max), 0xb000);

  constexpr Move promoted_sample{ a8, a8, P, q, false, false, false, false };
  EXPECT_EQ(Move::encode_move(promoted_sample), 0xa0000);

  constexpr Move capture_flag{ a8, a8, P, P, true, false, false, false };
  EXPECT_EQ(Move::encode_move(capture_flag), 0x100000);

  constexpr Move double_flag{ a8, a8, P, P, false, true, false, false };
  EXPECT_EQ(Move::encode_move(double_flag), 0x200000);

  constexpr Move enpassant_flag{ a8, a8, P, P, false, false, true, false };
  EXPECT_EQ(Move::encode_move(enpassant_flag), 0x400000);

  constexpr Move castling_flag{ a8, a8, P, P, false, false, false, true };
  EXPECT_EQ(Move::encode_move(castling_flag), 0x800000);
}

TEST_F(move_test_fixture, encode_decode_roundtrip_edges) {
  constexpr array<Move, 6> edge_moves{
    Move{ a8, a8, P, no_pieces, false, false, false, false }, Move{ h1, a8, k, no_pieces, true, true, true, true },
    Move{ a8, h1, Q, R, true, false, false, false },          Move{ d4, e5, n, q, false, true, false, true },
    Move{ e2, e4, P, N, false, true, false, false },          Move{ e7, e8, p, q, true, false, false, false },
  };

  for(const auto& move : edge_moves) {
    const int encoded = Move::encode_move(move);
    const Move decoded = Move::decode_move(encoded);
    const int reencoded = Move::encode_move(decoded);
    EXPECT_EQ(reencoded, encoded);
  }
}
