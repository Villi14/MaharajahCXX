#include "../headers/Bitboard.h"
#include "../headers/Board.h"
#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

using namespace std;
using namespace maharajah;

class board_test_fixture : public testing::Test {
  protected:
  void SetUp() override;

  public:
  Board board{};

/**
 * Resets a board to its initial state.
 *
 * @param board The board to reset
 * @param side The side to set for the board
 */
  static void reset_board(Board& board, const Colors side) {
    board.state.side = side;
    board.state.en_passant = no_square;
    board.state.castle = 0;
    board.state.bitboards.fill(zero);
    board.state.occupancies.fill(zero);
  }

 /**
 * Checks if a given move is present in a MoveList.
 *
 * @param list The MoveList to search in
 * @param move The move to search for
 * @return true if the move is present, false otherwise
 */
  static bool has_move(const MoveList& list, const int move) {
    for(size_t i{}; i < list.size(); ++i) {
      if(list[i] == move)
        return true;
    }
    return false;
  }

 /**
 * Dumps a MoveList to a string, with each move represented as a two-character string
 * (source square followed by target square) followed by a space. If the move is a promotion,
 * the promoted piece is appended to the string. For example, the string "e2e4"
 * represents the move from e2 to e4, while the string "e2e4q" represents the move
 * from e2 to e4, promoting to a queen.
 *
 * @param list The MoveList to dump
 * @return A string representation of the MoveList
 */
  static string dump_moves(const MoveList& list) {
    string out;
    for(size_t i{}; i < list.size(); ++i) {
      const int move = list[i];
      const auto src = Move::get_move_source(move);
      const auto dst = Move::get_move_target(move);
      const auto promo = Move::get_move_promoted(move);
      out += string(square_to_coord[src]) + string(square_to_coord[dst]);
      if(promo != no_pieces) {
        out += promoted_pieces[promo];
      }
      out += " ";
    }
    return out;
  }
};

/**
 * Initializes all bitboards and attack tables.
 */
void board_test_fixture::SetUp() {
  init_all();
}

/**
 * Tests that the copy_board method correctly copies the board state.
 *
 * This test initializes the board with all pieces on the board, then calls
 * the copy_board method. It then checks that the copy_state of the board
 * matches the current state of the board. Finally, it modifies the state of
 * the board and checks that the copy_state is not affected by the modifications.
 */
TEST_F(board_test_fixture, copy_board_test) {
  board.state.bitboards[P] = 0x000000000000FF00ULL;
  board.state.bitboards[N] = 0x0000000000000042ULL;
  board.state.bitboards[B] = 0x0000000000000024ULL;
  board.state.bitboards[R] = 0x0000000000000081ULL;
  board.state.bitboards[Q] = 0x0000000000000008ULL;
  board.state.bitboards[K] = 0x0000000000000010ULL;
  board.state.bitboards[p] = 0x00FF000000000000ULL;
  board.state.bitboards[n] = 0x4200000000000000ULL;
  board.state.bitboards[b] = 0x2400000000000000ULL;
  board.state.bitboards[r] = 0x8100000000000000ULL;
  board.state.bitboards[q] = 0x0800000000000000ULL;
  board.state.bitboards[k] = 0x1000000000000000ULL;
  board.state.side = black;
  board.state.en_passant = e3;
  board.state.castle = wk | bq;

  board.update_occupancies();

  board.copy_board();

  EXPECT_EQ(board.copy_state.bitboards, board.state.bitboards);
  EXPECT_EQ(board.copy_state.occupancies, board.state.occupancies);
  EXPECT_EQ(board.copy_state.side, board.state.side);
  EXPECT_EQ(board.copy_state.en_passant, board.state.en_passant);
  EXPECT_EQ(board.copy_state.castle, board.state.castle);

  // Ensure copy_state is not affected by subsequent mutations of state.
  board.state.bitboards[P] = zero;
  board.state.occupancies[both] = zero;
  board.state.side = white;
  board.state.en_passant = no_square;
  board.state.castle = zero;

  EXPECT_NE(board.copy_state.bitboards, board.state.bitboards);
  EXPECT_NE(board.copy_state.occupancies, board.state.occupancies);
  EXPECT_NE(board.copy_state.side, board.state.side);
  EXPECT_NE(board.copy_state.en_passant, board.state.en_passant);
  EXPECT_NE(board.copy_state.castle, board.state.castle);
}

/**
 * Tests that the take_back method correctly reverts the board state back to the copy_state.
 *
 * This test initializes the board with all pieces on the board, then calls
 * the copy_board method. It then modifies the state of the board and calls
 * the take_back method. Finally, it checks that the state of the board matches
 * the copy_state of the board.
 */
TEST_F(board_test_fixture, take_back_test) {
  board.state.bitboards[P] = 0x000000000000FF00ULL;
  board.state.bitboards[N] = 0x0000000000000042ULL;
  board.state.bitboards[B] = 0x0000000000000024ULL;
  board.state.bitboards[R] = 0x0000000000000081ULL;
  board.state.bitboards[Q] = 0x0000000000000008ULL;
  board.state.bitboards[K] = 0x0000000000000010ULL;
  board.state.bitboards[p] = 0x00FF000000000000ULL;
  board.state.bitboards[n] = 0x4200000000000000ULL;
  board.state.bitboards[b] = 0x2400000000000000ULL;
  board.state.bitboards[r] = 0x8100000000000000ULL;
  board.state.bitboards[q] = 0x0800000000000000ULL;
  board.state.bitboards[k] = 0x1000000000000000ULL;
  board.state.side = black;
  board.state.en_passant = e3;
  board.state.castle = wk | bq;

  board.update_occupancies();

  board.copy_board();

  // Change the board state
  board.state.bitboards[P] = zero;
  board.state.bitboards[k] = zero;
  board.state.occupancies[both] = zero;
  board.state.side = white;
  board.state.en_passant = no_square;
  board.state.castle = 0;

  board.take_back();

  EXPECT_EQ(board.state.bitboards, board.copy_state.bitboards);
  EXPECT_EQ(board.state.occupancies, board.copy_state.occupancies);
  EXPECT_EQ(board.state.side, board.copy_state.side);
  EXPECT_EQ(board.state.en_passant, board.copy_state.en_passant);
  EXPECT_EQ(board.state.castle, board.copy_state.castle);
}

/**
 * @brief Checks if a given square is attacked by a given side.
 *
 * @param side The side to check for attacks.
 * @param square The square to check for attacks.
 * @return true if the given square is attacked by the given side, false otherwise.
 */
TEST_F(board_test_fixture, is_square_attacked_test) {
  struct AttackCase {
    Pieces piece_white;
    Pieces piece_black;
    const char* name;
    bool is_slider;
  };

  constexpr array<AttackCase, 6> cases{ {
      { P, p, "pawn", false },
      { N, n, "knight", false },
      { B, b, "bishop", true },
      { R, r, "rook", true },
      { Q, q, "queen", true },
      { K, k, "king", false },
  } };

  for(const auto side : { white, black }) {
    for(const auto& test_case : cases) {
      const Pieces piece = (side == white) ? test_case.piece_white : test_case.piece_black;
      for(Squares square{ a8 }; square < no_square; ++square) {
        reset_board(board, side);
        board.state.bitboards[piece] = (one << square);
        board.update_occupancies();

        u64 expected{};
        if(piece == P || piece == p) {
          expected = pawn_attacks[side][square];
        } else if(piece == N || piece == n) {
          expected = knight_attacks[square];
        } else if(piece == B || piece == b) {
          expected = get_bishop_attacks(square, zero);
        } else if(piece == R || piece == r) {
          expected = get_rook_attacks(square, zero);
        } else if(piece == Q || piece == q) {
          expected = get_queen_attacks(square, zero);
        } else if(piece == K || piece == k) {
          expected = king_attacks[square];
        }

        for(Squares square{ a8 }; square < no_square; ++square) {
          {
            const bool exp = (expected & (one << square)) != zero;
            SCOPED_TRACE(testing::Message() << "side=" << (side == white ? "white" : "black") << " piece=" << test_case.name
                                            << " from=" << square_to_coord[square] << " to=" << square_to_coord[square]);
            EXPECT_EQ(board.is_square_attacked(square, side), exp);
          }
        }
      }
    }
  }
}

/**
 * @brief Checks if the generate_moves() function correctly generates all possible moves for a given
 * board state.
 *
 * This test function checks if the generate_moves() function correctly generates all possible moves
 * for all pieces on the board. It does this by setting a single piece on the board and then
 * checking if the generate_moves() function correctly generates all possible moves for that piece.
 *
 * @details The test function first checks if the generate_moves() function correctly generates all
 * possible moves for pawns. It then checks if the generate_moves() function correctly generates all
 * possible moves for knights, bishops, rooks, queens, and kings.
 */
TEST_F(board_test_fixture, generate_moves_test) {
  const auto expect_moves = [&](const vector<int>& expected) {
    EXPECT_EQ(board.moves_list.size(), expected.size()) << dump_moves(board.moves_list);
    for(const int move : expected) {
      EXPECT_TRUE(has_move(board.moves_list, move)) << dump_moves(board.moves_list);
    }
  };

  const auto set_single_piece = [&](const Colors side, const Pieces piece, const Squares square) {
    reset_board(board, side);
    board.state.bitboards[piece] = (one << square);
    board.update_occupancies();
    board.generate_moves();
  };

  const auto build_slider_attacks = [&](const Pieces piece, const Squares square) {
    if(piece == B || piece == b)
      return get_bishop_attacks(square, board.state.occupancies[both]);
    if(piece == R || piece == r)
      return get_rook_attacks(square, board.state.occupancies[both]);
    return get_queen_attacks(square, board.state.occupancies[both]);
  };

  // Pawns: all squares (empty board, no captures)
  for(Squares square{ a8 }; square < no_square; ++square) {
    vector<int> expected{};
    set_single_piece(white, P, square);

    const Squares target = square - rank_bit;
    if(target != no_square) {
      if(square >= a7 && square <= h7) {
        expected.push_back(Move::encode_move(Move{ square, target, P, Q, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, P, R, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, P, B, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, P, N, false, false, false, false }));
      } else {
        expected.push_back(Move::encode_move(Move{ square, target, P, no_pieces, false, false, false, false }));
        if(square >= a2 && square <= h2) {
          const Squares double_target = square - (rank_bit * 2);
          expected.push_back(Move::encode_move(Move{ square, double_target, P, no_pieces, false, true, false, false }));
        }
      }
    }

    expect_moves(expected);
  }

  for(Squares square{ a8 }; square < no_square; ++square) {
    vector<int> expected{};
    set_single_piece(black, p, square);

    const Squares target = square + rank_bit;
    if(target != no_square) {
      if(square >= a2 && square <= h2) {
        expected.push_back(Move::encode_move(Move{ square, target, p, q, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, p, r, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, p, b, false, false, false, false }));
        expected.push_back(Move::encode_move(Move{ square, target, p, n, false, false, false, false }));
      } else {
        expected.push_back(Move::encode_move(Move{ square, target, p, no_pieces, false, false, false, false }));
        if(square >= a7 && square <= h7) {
          const Squares double_target = square + (rank_bit * 2);
          expected.push_back(Move::encode_move(Move{ square, double_target, p, no_pieces, false, true, false, false }));
        }
      }
    }

    expect_moves(expected);
  }

  // Knights
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_single_piece(white, N, square);
    vector<int> expected{};
    u64 attacks = knight_attacks[square] & ~board.state.occupancies[white];
    while(attacks) {
      const Squares target = get_ls1b_index(attacks);
      expected.push_back(Move::encode_move(Move{ square, target, N, no_pieces, false, false, false, false }));
      pop_bit(attacks, target);
    }
    expect_moves(expected);
  }

  for(Squares square{ a8 }; square < no_square; ++square) {
    set_single_piece(black, n, square);
    vector<int> expected{};
    u64 attacks = knight_attacks[square] & ~board.state.occupancies[black];
    while(attacks) {
      const Squares target = get_ls1b_index(attacks);
      expected.push_back(Move::encode_move(Move{ square, target, n, no_pieces, false, false, false, false }));
      pop_bit(attacks, target);
    }
    expect_moves(expected);
  }

  // Bishops, rooks, queens
  for(const Pieces piece : { B, R, Q }) {
    for(Squares square{ a8 }; square < no_square; ++square) {
      set_single_piece(white, piece, square);
      vector<int> expected{};
      u64 attacks = build_slider_attacks(piece, square) & ~board.state.occupancies[white];
      while(attacks) {
        const Squares target = get_ls1b_index(attacks);
        expected.push_back(Move::encode_move(Move{ square, target, piece, no_pieces, false, false, false, false }));
        pop_bit(attacks, target);
      }
      expect_moves(expected);
    }
  }

  for(const Pieces piece : { b, r, q }) {
    for(Squares square{ a8 }; square < no_square; ++square) {
      set_single_piece(black, piece, square);
      vector<int> expected{};
      u64 attacks = build_slider_attacks(piece, square) & ~board.state.occupancies[black];
      while(attacks) {
        const Squares target = get_ls1b_index(attacks);
        expected.push_back(Move::encode_move(Move{ square, target, piece, no_pieces, false, false, false, false }));
        pop_bit(attacks, target);
      }
      expect_moves(expected);
    }
  }

  // Kings (castling is covered elsewhere)
  for(Squares square{ a8 }; square < no_square; ++square) {
    set_single_piece(white, K, square);
    vector<int> expected{};
    u64 attacks = king_attacks[square] & ~board.state.occupancies[white];
    while(attacks) {
      const Squares target = get_ls1b_index(attacks);
      expected.push_back(Move::encode_move(Move{ square, target, K, no_pieces, false, false, false, false }));
      pop_bit(attacks, target);
    }
    expect_moves(expected);
  }

  for(Squares square{ a8 }; square < no_square; ++square) {
    set_single_piece(black, k, square);
    vector<int> expected{};
    u64 attacks = king_attacks[square] & ~board.state.occupancies[black];
    while(attacks) {
      const Squares target = get_ls1b_index(attacks);
      expected.push_back(Move::encode_move(Move{ square, target, k, no_pieces, false, false, false, false }));
      pop_bit(attacks, target);
    }
    expect_moves(expected);
  }
}

/**
 * @brief Test that the board generates the correct moves when castling by attacking a square.
 * @details Test that the board generates the correct moves when castling by attacking a square, and that it does not generate moves blocked by pieces of the same color.
 */
TEST_F(board_test_fixture, generate_moves_white_castling) {
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.bitboards[R] = (one << h1);
  board.state.castle = wk;
  board.update_occupancies();
  board.generate_moves();

  const int e1g1 = Move::encode_move(Move{ e1, g1, K, no_pieces, false, false, false, true });
  EXPECT_TRUE(has_move(board.moves_list, e1g1));

  // Block castling by attacking g1
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.bitboards[R] = (one << h1);
  board.state.bitboards[r] = (one << g8);
  board.state.castle = wk;
  board.update_occupancies();
  board.generate_moves();

  EXPECT_FALSE(has_move(board.moves_list, e1g1));

  // Queenside castling
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.bitboards[R] = (one << a1);
  board.state.castle = wq;
  board.update_occupancies();
  board.generate_moves();

  const int e1c1 = Move::encode_move(Move{ e1, c1, K, no_pieces, false, false, false, true });
  EXPECT_TRUE(has_move(board.moves_list, e1c1));

  // Block queenside castling by attacking c1
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.bitboards[R] = (one << a1);
  board.state.bitboards[r] = (one << c8);
  board.state.castle = wq;
  board.update_occupancies();
  board.generate_moves();

  EXPECT_FALSE(has_move(board.moves_list, e1c1));
}

// Tests that the board generates moves for a king that is blocked by pieces of the same color and castling type.
TEST_F(board_test_fixture, generate_moves_black_castling) {
  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.bitboards[r] = (one << h8);
  board.state.castle = bk;
  board.update_occupancies();
  board.generate_moves();

  const int e8g8 = Move::encode_move(Move{ e8, g8, k, no_pieces, false, false, false, true });
  EXPECT_TRUE(has_move(board.moves_list, e8g8));

  // Block castling by attacking g1
  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.bitboards[r] = (one << h8);
  board.state.bitboards[R] = (one << g1);
  board.state.castle = bk;
  board.update_occupancies();
  board.generate_moves();

  EXPECT_FALSE(has_move(board.moves_list, e8g8));

  // Queenside castling
  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.bitboards[r] = (one << a8);
  board.state.castle = bq;
  board.update_occupancies();
  board.generate_moves();

  const int e8c8 = Move::encode_move(Move{ e8, c8, k, no_pieces, false, false, false, true });
  EXPECT_TRUE(has_move(board.moves_list, e8c8));

  // Block queenside castling by attacking c8
  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.bitboards[r] = (one << a8);
  board.state.bitboards[R] = (one << c1);
  board.state.castle = bq;
  board.update_occupancies();
  board.generate_moves();

  EXPECT_FALSE(has_move(board.moves_list, e8c8));
}

/**
 * @brief Tests that the board updates the state correctly after a move that updates the castling rights.
 *
 * @details This test checks that the board updates the state correctly after a move that updates the castling rights.
 *              It checks that the castling rights are updated correctly after a king move and a rook move.
 */
TEST_F(board_test_fixture, make_move_updates_white_castling_rights) {
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.castle = wk | wq;
  board.update_occupancies();

  const int king_move = Move::encode_move(Move{ e1, e2, K, no_pieces, false, false, false, false });
  board.make_move(king_move, TypeMove::all_moves);
  EXPECT_EQ(board.state.castle, 0);

  reset_board(board, white);
  board.state.bitboards[R] = (one << h1);
  board.state.castle = wk | wq;
  board.update_occupancies();

  const int rook_move = Move::encode_move(Move{ h1, h2, R, no_pieces, false, false, false, false });
  board.make_move(rook_move, TypeMove::all_moves);
  EXPECT_EQ(board.state.castle & wk, 0);
  EXPECT_NE(board.state.castle & wq, 0);
}

/**
 * @brief Tests that the board updates the state correctly after a move that updates the castling rights.
 * @details This test checks that the board updates the state correctly after a move that updates the castling rights.
 *              It checks that the castling rights are updated correctly after a king move and a rook move.
 */
TEST_F(board_test_fixture, make_move_updates_black_castling_rights) {
  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.castle = bk | bq;
  board.update_occupancies();

  const int king_move = Move::encode_move(Move{ e8, e7, k, no_pieces, false, false, false, false });
  board.make_move(king_move, TypeMove::all_moves);
  EXPECT_EQ(board.state.castle, 0);

  reset_board(board, black);
  board.state.bitboards[r] = (one << h8);
  board.state.castle = bk | bq;
  board.update_occupancies();

  const int rook_move = Move::encode_move(Move{ h8, h7, r, no_pieces, false, false, false, false });
  board.make_move(rook_move, TypeMove::all_moves);
  EXPECT_EQ(board.state.castle & bk, 0);
  EXPECT_NE(board.state.castle & bq, 0);
}

/**
 * @brief Tests that the board generates the correct moves when en passant is set.
 * @details Tests that the board generates the correct moves when en passant is set, and that it does not generate moves blocked by pieces of the same color.
 */
TEST_F(board_test_fixture, generate_moves_white_en_passant) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << e5);
  board.state.bitboards[p] = (one << d5);
  board.state.en_passant = d6;
  board.update_occupancies();
  board.generate_moves();

  const int e5d6 = Move::encode_move(Move{ e5, d6, P, no_pieces, true, false, true, false });
  EXPECT_TRUE(has_move(board.moves_list, e5d6));
}

/**
 * @brief Tests that the board generates the correct moves when en passant is set for black pieces.
 * @details Tests that the board generates the correct moves when en passant is set for black pieces, and that it does not generate moves blocked by pieces of the same color.
 */
TEST_F(board_test_fixture, generate_moves_black_en_passant) {
  reset_board(board, black);
  board.state.bitboards[p] = (one << d4);
  board.state.bitboards[P] = (one << e4);
  board.state.en_passant = e3;
  board.update_occupancies();
  board.generate_moves();

  const int d4e3 = Move::encode_move(Move{ d4, e3, p, no_pieces, true, false, true, false });
  EXPECT_TRUE(has_move(board.moves_list, d4e3));
}

/**
 * @brief Tests that the board generates the correct moves when a pawn is blocked.
 * @details Tests that the board generates the correct moves when a pawn is blocked by a piece of the same color, and that it does not generate moves blocked by pieces of the same color.
 */
TEST_F(board_test_fixture, generate_moves_blocked_white_pawn) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << e2);
  board.state.bitboards[p] = (one << e3);
  board.update_occupancies();
  board.generate_moves();

  EXPECT_EQ(board.moves_list.size(), 0);

  reset_board(board, black);
  board.state.bitboards[p] = (one << e7);
  board.state.bitboards[P] = (one << e6);
  board.update_occupancies();
  board.generate_moves();

  EXPECT_EQ(board.moves_list.size(), 0);
}

/**
 * @brief Test that the board generates the correct moves for a pawn that captures a piece.
 * @details Test that the board generates the correct moves for a pawn that captures a piece, and that it does not generate moves blocked by pieces of the same color.
 */
TEST_F(board_test_fixture, generate_moves_pawn_captures) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << d4);
  board.state.bitboards[p] = (one << c5);
  board.update_occupancies();
  board.generate_moves();

  const int d4c5 = Move::encode_move(Move{ d4, c5, P, no_pieces, true, false, false, false });
  EXPECT_TRUE(has_move(board.moves_list, d4c5));

  reset_board(board, black);
  board.state.bitboards[p] = (one << e5);
  board.state.bitboards[P] = (one << f4);
  board.update_occupancies();
  board.generate_moves();

  const int e5f4 = Move::encode_move(Move{ e5, f4, p, no_pieces, true, false, false, false });
  EXPECT_TRUE(has_move(board.moves_list, e5f4));
}

/**
 * @brief Test that the board generates the correct moves for a pawn that promotes to a different piece.
 * @details Test that the board generates the correct moves for a pawn that promotes to a different piece.
 */
TEST_F(board_test_fixture, generate_moves_promotion) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << a7);
  board.update_occupancies();
  board.generate_moves();

  EXPECT_EQ(board.moves_list.size(), 4);
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ a7, a8, P, Q, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ a7, a8, P, R, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ a7, a8, P, B, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ a7, a8, P, N, false, false, false, false })));

  reset_board(board, black);
  board.state.bitboards[p] = (one << h2);
  board.update_occupancies();
  board.generate_moves();

  EXPECT_EQ(board.moves_list.size(), 4);
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ h2, h1, p, q, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ h2, h1, p, r, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ h2, h1, p, b, false, false, false, false })));
  EXPECT_TRUE(has_move(board.moves_list, Move::encode_move(Move{ h2, h1, p, n, false, false, false, false })));
}

/**
 * @brief Test that the board does not generate moves for a pawn that is blocked by pieces of the same color when castling.
 * @details Test that the board does not generate moves for a pawn that is blocked by pieces of the same color when castling.
 */
TEST_F(board_test_fixture, generate_moves_castling_blocked_by_pieces) {
  reset_board(board, white);
  board.state.bitboards[K] = (one << e1);
  board.state.bitboards[R] = (one << h1);
  board.state.bitboards[N] = (one << f1); // block
  board.state.castle = wk;
  board.update_occupancies();
  board.generate_moves();

  const int e1g1 = Move::encode_move(Move{ e1, g1, K, no_pieces, false, false, false, true });
  EXPECT_FALSE(has_move(board.moves_list, e1g1));

  reset_board(board, black);
  board.state.bitboards[k] = (one << e8);
  board.state.bitboards[r] = (one << h8);
  board.state.bitboards[n] = (one << f8); // block
  board.state.castle = bk;
  board.update_occupancies();
  board.generate_moves();

  const int e8g8 = Move::encode_move(Move{ e8, g8, k, no_pieces, false, false, false, true });
  EXPECT_FALSE(has_move(board.moves_list, e8g8));
}

/**
 * @brief Tests that the board generates the correct moves for a slider (rook or bishop) when there are blockers on its path.
 * @details Test that the board generates the correct moves for a slider (rook or bishop) when there are blockers on its path.
 */
TEST_F(board_test_fixture, generate_moves_sliders_with_blockers) {
  reset_board(board, white);
  board.state.bitboards[R] = (one << a1);
  board.state.bitboards[P] = (one << a2); // block rook
  board.update_occupancies();
  board.generate_moves();

  const int a1a2 = Move::encode_move(Move{ a1, a2, R, no_pieces, false, false, false, false });
  EXPECT_FALSE(has_move(board.moves_list, a1a2));

  reset_board(board, white);
  board.state.bitboards[B] = (one << c1);
  board.state.bitboards[P] = (one << d2); // block bishop diagonal
  board.update_occupancies();
  board.generate_moves();

  const int c1d2 = Move::encode_move(Move{ c1, d2, B, no_pieces, false, false, false, false });
  EXPECT_FALSE(has_move(board.moves_list, c1d2));

  reset_board(board, black);
  board.state.bitboards[r] = (one << a8);
  board.state.bitboards[p] = (one << a7); // block rook
  board.update_occupancies();
  board.generate_moves();

  const int a8a7 = Move::encode_move(Move{ a8, a7, r, no_pieces, false, false, false, false });
  EXPECT_FALSE(has_move(board.moves_list, a8a7));

  reset_board(board, black);
  board.state.bitboards[b] = (one << c8);
  board.state.bitboards[p] = (one << d7); // block bishop diagonal
  board.update_occupancies();
  board.generate_moves();

  const int c8d7 = Move::encode_move(Move{ c8, d7, b, no_pieces, false, false, false, false });
  EXPECT_FALSE(has_move(board.moves_list, c8d7));
}

/**
 * @brief Test that a square is attacked by a slider.
 *
 * @details This test checks that a square is attacked by a rook, bishop, or queen.
 *              It also checks that a square is not attacked by a piece of the opposite color.
 */
TEST_F(board_test_fixture, is_square_attacked_by_sliders) {
  reset_board(board, white);
  board.state.bitboards[R] = (one << a1);
  board.state.bitboards[B] = (one << c1);
  board.state.bitboards[Q] = (one << d1);
  board.update_occupancies();

  EXPECT_TRUE(board.is_square_attacked(a8, white)); // rook file
  EXPECT_TRUE(board.is_square_attacked(h6, white)); // bishop diagonal
  EXPECT_TRUE(board.is_square_attacked(d8, white)); // queen file
  EXPECT_FALSE(board.is_square_attacked(b2, black));

  reset_board(board, black);
  board.state.bitboards[r] = (one << a8);
  board.state.bitboards[b] = (one << c8);
  board.state.bitboards[q] = (one << d8);
  board.update_occupancies();

  EXPECT_TRUE(board.is_square_attacked(a1, black)); // rook file
  EXPECT_TRUE(board.is_square_attacked(h3, black)); // bishop diagonal
  EXPECT_TRUE(board.is_square_attacked(d1, black)); // queen file
  EXPECT_FALSE(board.is_square_attacked(b7, white));
}

/**
 * @brief Test that the board updates the state correctly after a capture move.
 *
 * @details This test checks that the board updates the state correctly after a capture move.
 *          It checks that the capturing piece is moved to the destination square, that the captured piece is removed from the board,
 *          and that the side is updated correctly.
 */
TEST_F(board_test_fixture, make_move_capture) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << e4);
  board.state.bitboards[p] = (one << d5);
  board.update_occupancies();

  const int move = Move::encode_move(Move{ e4, d5, P, no_pieces, true, false, false, false });
  board.make_move(move, TypeMove::all_moves);

  EXPECT_TRUE(get_bit(board.state.bitboards[P], d5));
  EXPECT_FALSE(get_bit(board.state.bitboards[p], d5));
  EXPECT_EQ(board.state.side, black);
  EXPECT_EQ(board.state.en_passant, no_square);

  reset_board(board, black);
  board.state.bitboards[p] = (one << d5);
  board.state.bitboards[P] = (one << e4);
  board.update_occupancies();

  const int black_move = Move::encode_move(Move{ d5, e4, p, no_pieces, true, false, false, false });
  board.make_move(black_move, TypeMove::all_moves);

  EXPECT_TRUE(get_bit(board.state.bitboards[p], e4));
  EXPECT_FALSE(get_bit(board.state.bitboards[P], e4));
  EXPECT_EQ(board.state.side, white);
  EXPECT_EQ(board.state.en_passant, no_square);
}

/**
 * @brief Test that the board updates the state correctly after an en passant move.
 *
 * @details This test checks that the board updates the state correctly after an en passant move.
 *              It checks that the pawn is moved to the destination square, that the captured pawn is removed from the board,
 *              and that the side and en passant are updated correctly.
 */
TEST_F(board_test_fixture, make_move_en_passant) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << e5);
  board.state.bitboards[p] = (one << d5);
  board.state.en_passant = d6;
  board.update_occupancies();

  const int move = Move::encode_move(Move{ e5, d6, P, no_pieces, true, false, true, false });
  board.make_move(move, TypeMove::all_moves);

  EXPECT_TRUE(get_bit(board.state.bitboards[P], d6));
  EXPECT_FALSE(get_bit(board.state.bitboards[p], d5));
  EXPECT_EQ(board.state.side, black);
  EXPECT_EQ(board.state.en_passant, no_square);

  reset_board(board, black);
  board.state.bitboards[p] = (one << d4);
  board.state.bitboards[P] = (one << e4);
  board.state.en_passant = e3;
  board.update_occupancies();

  const int black_move = Move::encode_move(Move{ d4, e3, p, no_pieces, true, false, true, false });
  board.make_move(black_move, TypeMove::all_moves);

  EXPECT_TRUE(get_bit(board.state.bitboards[p], e3));
  EXPECT_FALSE(get_bit(board.state.bitboards[P], e4));
  EXPECT_EQ(board.state.side, white);
  EXPECT_EQ(board.state.en_passant, no_square);
}

/**
 * @brief Tests that the board updates the state correctly after a promotion move.
 *              It checks that the pawn is moved to the destination square, that the captured pawn is removed from the board,
 *              and that the side is updated correctly.
 */
TEST_F(board_test_fixture, make_move_promotion) {
  reset_board(board, white);
  board.state.bitboards[P] = (one << a7);
  board.update_occupancies();

  const int move = Move::encode_move(Move{ a7, a8, P, Q, false, false, false, false });
  board.make_move(move, TypeMove::all_moves);

  EXPECT_FALSE(get_bit(board.state.bitboards[P], a8));
  EXPECT_TRUE(get_bit(board.state.bitboards[Q], a8));

  reset_board(board, black);
  board.state.bitboards[p] = (one << a2);
  board.update_occupancies();

  const int black_move = Move::encode_move(Move{ a2, a1, p, q, false, false, false, false });
  board.make_move(black_move, TypeMove::all_moves);

  EXPECT_FALSE(get_bit(board.state.bitboards[p], a1));
  EXPECT_TRUE(get_bit(board.state.bitboards[q], a1));
}
