// Move-generation and rules regressions ported from MaharajahC's
// compound_piece_smoke.c, engine_rules_smoke.c, special_moves_smoke.c and
// pawn_unmoved_smoke.c. FENs are kept verbatim, including the trailing spaces.

#include "../headers/Bitboard.h"
#include "../headers/Engine.h"
#include "../headers/Move.h"
#include "../headers/Zobrist.h"
#include "gtest/gtest.h"

using namespace std;
using namespace maharajah;

class rules_test_fixture : public testing::Test {
  public:
  Engine engine{ };
  Board& board = engine.board;

  int parse(const char* move) const {
    return board.parse_move(move);
  }

  // parses and plays a move; false if it does not parse or is illegal
  bool play(const char* move) {
    const int parsed = parse(move);
    return parsed != 0 && board.make_move(parsed, TypeMove::all_moves);
  }

  int count_moves_from(const Squares source) const {
    MoveList list;
    board.generate_moves(list);
    int count{ };
    for(size_t i{ }; i < list.size(); ++i)
      if(Move::get_move_source(list[i]) == source)
        ++count;
    return count;
  }

  int count_legal_moves() {
    MoveList list;
    board.generate_moves(list);
    int count{ };
    for(size_t i{ }; i < list.size(); ++i) {
      if(board.make_move(list[i], TypeMove::all_moves)) {
        ++count;
        board.pop_state();
      }
    }
    return count;
  }

  bool hash_consistent() const {
    return board.state.hash_key == generate_hash_key(board.state);
  }
};

// compound_piece_smoke.c

TEST_F(rules_test_fixture, compound_pieces_load_with_consistent_hash_and_move_counts) {
  board.parse_fen("k7/8/8/8/3A4/8/8/7K w - - 0 1 ");
  EXPECT_TRUE(get_bit(board.state.bitboards[A], d4));
  EXPECT_TRUE(hash_consistent());
  EXPECT_EQ(count_moves_from(d4), 21);

  board.parse_fen("k7/8/8/8/3C4/8/8/7K w - - 0 1 ");
  EXPECT_EQ(count_moves_from(d4), 22);
  EXPECT_TRUE(hash_consistent());

  board.parse_fen("k7/8/8/8/3M4/8/8/7K w - - 0 1 ");
  EXPECT_EQ(count_moves_from(d4), 35);
  EXPECT_TRUE(hash_consistent());
}

TEST_F(rules_test_fixture, compound_move_hash_matches_the_same_position_from_fen) {
  board.parse_fen("k7/8/8/8/3A4/8/8/7K w - - 0 1 ");
  ASSERT_TRUE(play("d4f5"));
  EXPECT_TRUE(hash_consistent());

  const u64 move_hash = board.state.hash_key;
  board.parse_fen("k7/8/8/5A2/8/8/8/7K b - - 0 1 ");
  EXPECT_EQ(board.state.hash_key, move_hash);
}

TEST_F(rules_test_fixture, amazon_promotion_hash_matches_the_same_position_from_fen) {
  // field 7 declares White variant: the bare board has no compound piece to infer it from
  board.parse_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 V");
  const int promotion = parse("a7a8m");
  ASSERT_NE(promotion, 0);
  EXPECT_EQ(Move::get_move_promoted(promotion), M);
  ASSERT_TRUE(board.make_move(promotion, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[M], a8));
  EXPECT_TRUE(hash_consistent());

  const u64 promotion_hash = board.state.hash_key;
  board.parse_fen("M6k/8/8/8/8/8/8/K7 b - - 0 1 ");
  EXPECT_EQ(board.state.hash_key, promotion_hash);
}

// engine_rules_smoke.c

TEST_F(rules_test_fixture, compound_pieces_attack_with_every_component) {
  board.parse_fen("k7/8/1A6/8/8/8/8/7K b - - 0 1 ");
  EXPECT_TRUE(board.is_square_attacked(a8, white)); // archbishop knight hop

  board.parse_fen("2k5/8/8/8/2C5/8/8/K7 b - - 0 1 ");
  EXPECT_TRUE(board.is_square_attacked(c8, white)); // chancellor file

  board.parse_fen("7k/8/8/8/3M4/8/8/K7 b - - 0 1 ");
  EXPECT_TRUE(board.is_square_attacked(h8, white)); // amazon diagonal
}

TEST_F(rules_test_fixture, pinned_archbishop_may_not_move) {
  board.parse_fen("4r2k/8/8/8/8/8/4A3/4K3 w - - 0 1 ");
  const int move = parse("e2g3");
  ASSERT_NE(move, 0);
  EXPECT_FALSE(board.make_move(move, TypeMove::all_moves));
}

TEST_F(rules_test_fixture, variant_side_promotes_to_archbishop_and_chancellor) {
  board.parse_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 V");
  int promotion = parse("a7a8a");
  ASSERT_NE(promotion, 0);
  EXPECT_EQ(Move::get_move_promoted(promotion), A);
  ASSERT_TRUE(board.make_move(promotion, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[A], a8));

  board.parse_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 V");
  promotion = parse("a7a8c");
  ASSERT_NE(promotion, 0);
  EXPECT_EQ(Move::get_move_promoted(promotion), C);
  ASSERT_TRUE(board.make_move(promotion, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[C], a8));
}

TEST_F(rules_test_fixture, amazon_mate_and_queen_stalemate_have_no_legal_moves) {
  board.parse_fen("k7/1M6/2K5/8/8/8/8/8 b - - 0 1 ");
  EXPECT_TRUE(board.is_square_attacked(a8, white));
  EXPECT_EQ(count_legal_moves(), 0);

  board.parse_fen("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1 ");
  EXPECT_FALSE(board.is_square_attacked(a8, white));
  EXPECT_EQ(count_legal_moves(), 0);
}

TEST_F(rules_test_fixture, mixed_game_compound_promotion_only_for_the_variant_side) {
  // field 7 "v": only Black plays variant rules
  board.parse_fen("7k/P7/8/8/8/8/8/K7 w - - 0 1 v");
  EXPECT_EQ(parse("a7a8a"), 0);
  EXPECT_EQ(parse("a7a8c"), 0);
  EXPECT_EQ(parse("a7a8m"), 0);
  EXPECT_NE(parse("a7a8q"), 0);

  board.parse_fen("K7/8/8/8/8/8/p7/7k b - - 0 1 v");
  const int promotion = parse("a2a1a");
  ASSERT_NE(promotion, 0);
  EXPECT_EQ(Move::get_move_promoted(promotion), a);
}

TEST_F(rules_test_fixture, mixed_game_classic_side_keeps_castling) {
  // Black fields an archbishop, but only Black plays variant: White may still castle
  board.parse_fen("1A2k3/8/8/8/8/8/8/R3K2R w KQ - 0 1 v");
  const int kingside = parse("e1g1");
  ASSERT_NE(kingside, 0);
  EXPECT_TRUE(Move::get_move_castling(kingside));
  ASSERT_TRUE(board.make_move(kingside, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[K], g1));
  EXPECT_TRUE(get_bit(board.state.bitboards[R], f1));

  board.parse_fen("1A2k3/8/8/8/8/8/8/R3K2R w KQ - 0 1 v");
  const int queenside = parse("e1c1");
  ASSERT_NE(queenside, 0);
  EXPECT_TRUE(Move::get_move_castling(queenside));
}

TEST_F(rules_test_fixture, variant_side_never_castles) {
  board.parse_fen("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1 V");
  EXPECT_EQ(board.state.castle, 0);
  EXPECT_EQ(parse("e1g1"), 0);
  EXPECT_EQ(parse("e1c1"), 0);
}

// special_moves_smoke.c

TEST_F(rules_test_fixture, castling_moves_king_and_rook) {
  board.parse_fen("4k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1 ");
  const int kingside = parse("e1g1");
  const int queenside = parse("e1c1");
  ASSERT_NE(kingside, 0);
  ASSERT_NE(queenside, 0);
  EXPECT_TRUE(Move::get_move_castling(kingside));
  EXPECT_TRUE(Move::get_move_castling(queenside));
  ASSERT_TRUE(board.make_move(kingside, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[K], g1));
  EXPECT_TRUE(get_bit(board.state.bitboards[R], f1));
}

TEST_F(rules_test_fixture, en_passant_next_to_a_compound_piece) {
  board.parse_fen("7k/8/8/3pP3/2A5/8/8/K7 w - d6 0 1 ");
  const int en_passant = parse("e5d6");
  ASSERT_NE(en_passant, 0);
  EXPECT_TRUE(Move::get_move_enpassant(en_passant));
  ASSERT_TRUE(board.make_move(en_passant, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[P], d6));
  EXPECT_FALSE(get_bit(board.state.bitboards[p], d5));
}

TEST_F(rules_test_fixture, no_castling_with_an_amazon_on_the_board) {
  board.parse_fen("4k3/8/8/8/8/8/3M4/R3K2R w KQ - 0 1 ");
  EXPECT_EQ(parse("e1g1"), 0);
}

TEST_F(rules_test_fixture, variant_pawn_double_steps_from_the_first_rank) {
  // the position the online server wrongly rejected ("the move is not legal")
  board.parse_fen("1aak3c/5n2/8/6n1/7M/6C1/8/2P1KP1P w - - 2 8 ");
  const int double_step = parse("f1f3");
  ASSERT_NE(double_step, 0);
  EXPECT_TRUE(Move::get_move_double(double_step));
  ASSERT_TRUE(board.make_move(double_step, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[P], f3));
  EXPECT_EQ(board.state.en_passant, f2);
}

TEST_F(rules_test_fixture, standard_pawn_past_its_home_rank_does_not_double_step) {
  board.parse_fen("4k3/8/8/8/8/4P3/8/4K3 w - - 0 1 ");
  EXPECT_EQ(parse("e3e5"), 0);
}

TEST_F(rules_test_fixture, compound_promotion_only_under_variant_rules) {
  board.parse_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1 ");
  EXPECT_NE(parse("a7a8q"), 0);
  EXPECT_EQ(parse("a7a8m"), 0);

  // a compound piece on the board switches to variant rules
  board.parse_fen("4k3/P7/8/8/7M/8/8/4K3 w - - 0 1 ");
  EXPECT_NE(parse("a7a8m"), 0);
}

// pawn_unmoved_smoke.c

TEST_F(rules_test_fixture, field_eight_dash_forbids_the_double_step_of_a_moved_pawn) {
  // the LAN automation hang (match 8, ply 87): the h-pawn already walked h7-h6
  board.parse_fen("8/5p2/7p/1K6/4a3/5k2/8/8 b - - 25 44 Vv -");
  EXPECT_TRUE(board.state.has_pawn_state);
  const int double_step = parse("h6h4");
  EXPECT_TRUE(double_step == 0 || !Move::get_move_double(double_step));
}

TEST_F(rules_test_fixture, field_eight_square_allows_one_double_step) {
  board.parse_fen("8/5p2/7p/1K6/4a3/5k2/8/8 b - - 25 44 Vv h6");
  const int double_step = parse("h6h4");
  ASSERT_NE(double_step, 0);
  EXPECT_TRUE(Move::get_move_double(double_step));
  ASSERT_TRUE(board.make_move(double_step, TypeMove::all_moves));
  EXPECT_TRUE(get_bit(board.state.bitboards[p], h4));
  // the double step spent the pawn's right
  EXPECT_FALSE(get_bit(board.state.pawn_unmoved, h6));

  ASSERT_TRUE(play("b5b4"));
  const int second_double = parse("h4h2");
  EXPECT_TRUE(second_double == 0 || !Move::get_move_double(second_double));
}

// Without field 8 a variant side's pawns on its own half count as unmoved (an army
// may start anywhere there), a standard side's on its home rank.
TEST_F(rules_test_fixture, without_field_eight_pawns_on_their_own_half_are_unmoved) {
  board.parse_fen("8/5p2/7p/1K6/4a3/5k2/8/8 b - - 25 44 Vv");
  EXPECT_TRUE(board.state.has_pawn_state);
  const int double_step = parse("h6h4");
  ASSERT_NE(double_step, 0);
  EXPECT_TRUE(Move::get_move_double(double_step));

  board.parse_fen("4k3/8/8/2P5/8/8/8/4K3 w - - 0 1 Vv");
  EXPECT_EQ(parse("c5c7"), 0);

  // white plays standard rules: only its home-rank pawn may double-step
  board.parse_fen("4k3/8/8/8/8/2P5/1P6/4K3 w - - 0 1 v");
  EXPECT_NE(parse("b2b4"), 0);
  EXPECT_EQ(parse("c3c5"), 0);
}

// a pawn double-steps only once, also when the FEN does not list the unmoved pawns
TEST_F(rules_test_fixture, a_variant_pawn_double_steps_only_once) {
  board.parse_fen("4k3/8/8/8/P7/8/8/4K3 w - - 0 1 Vv");
  ASSERT_TRUE(play("a4a6"));
  ASSERT_TRUE(play("e8d8"));
  EXPECT_EQ(parse("a6a8"), 0);
  EXPECT_NE(parse("a6a7"), 0);
  EXPECT_FALSE(get_bit(board.state.pawn_unmoved, a6));

  // the same through the FFI rules profile on a FEN without variant rights
  board.parse_fen("4k3/8/8/8/P7/8/8/4K3 w - - 0 1");
  board.state.side_variant = { true, true };
  board.state.infer_pawn_state();
  ASSERT_TRUE(play("a4a6"));
  ASSERT_TRUE(play("e8d8"));
  EXPECT_EQ(parse("a6a8"), 0);
}
