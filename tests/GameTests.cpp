#include "../headers/Game.h"
#include "TestAccess.h"
#include "TestPositions.h"
#include "gtest/gtest.h"

#include <map>

using namespace std;
using namespace maharajah;

map<string_view, string> test_fens = {
#ifdef _MSC_VER
  { TestFen::empty_board,
    R"(
 8  . . . . . . . .
 7  . . . . . . . .
 6  . . . . . . . .
 5  . . . . . . . .
 4  . . . . . . . .
 3  . . . . . . . .
 2  . . . . . . . .
 1  . . . . . . . .

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  ----

)" },

  { Fen::start_position,
    R"(
 8  r n b q k b n r
 7  p p p p p p p p
 6  . . . . . . . .
 5  . . . . . . . .
 4  . . . . . . . .
 3  . . . . . . . .
 2  P P P P P P P P
 1  R N B Q K B N R

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  KQkq

)" },

  { TestFen::tricky_position,
    R"(
 8  r . . . k . . r
 7  p . p p q p b .
 6  b n . . p n p .
 5  . . . P N . . .
 4  . p . . P . . .
 3  . . N . . Q . p
 2  P P P B B P P P
 1  R . . . K . . R

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  KQkq

)" },

  { TestFen::killer_position,
    R"(
 8  r n b q k b . r
 7  p p . p . p P p
 6  . . . . . . . .
 5  . . p . p P . .
 4  . P . P . . . .
 3  . . . P . . . P
 2  P . P . P . . .
 1  R N B Q K B N R

    a b c d e f g h

    Side:     white
    En passant:  e6
    Castling:  KQkq

)" },

  { TestFen::cmk_position,
    R"(
 8  r . . q . r k .
 7  p p p . . p p p
 6  . . n . b n . .
 5  . . b . p . . .
 4  . . . p P . . .
 3  . . . P . N P P
 2  P P P . N P B .
 1  R . B Q . R K .

    a b c d e f g h

    Side:     black
    En passant:  no
    Castling:  ----

)" }
#else
  { TestFen::empty_board,
    R"(
 8  . . . . . . . .
 7  . . . . . . . .
 6  . . . . . . . .
 5  . . . . . . . .
 4  . . . . . . . .
 3  . . . . . . . .
 2  . . . . . . . .
 1  . . . . . . . .

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  ----

)" },

  { Fen::start_position,
    R"(
 8  ♜ ♞ ♝ ♛ ♚ ♝ ♞ ♜
 7  ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎
 6  . . . . . . . .
 5  . . . . . . . .
 4  . . . . . . . .
 3  . . . . . . . .
 2  ♙ ♙ ♙ ♙ ♙ ♙ ♙ ♙
 1  ♖ ♘ ♗ ♕ ♔ ♗ ♘ ♖

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  KQkq

)" },

  { TestFen::tricky_position,
    R"(
 8  ♜ . . . ♚ . . ♜
 7  ♟︎ . ♟︎ ♟︎ ♛ ♟︎ ♝ .
 6  ♝ ♞ . . ♟︎ ♞ ♟︎ .
 5  . . . ♙ ♘ . . .
 4  . ♟︎ . . ♙ . . .
 3  . . ♘ . . ♕ . ♟︎
 2  ♙ ♙ ♙ ♗ ♗ ♙ ♙ ♙
 1  ♖ . . . ♔ . . ♖

    a b c d e f g h

    Side:     white
    En passant:  no
    Castling:  KQkq

)" },

  { TestFen::killer_position,
    R"(
 8  ♜ ♞ ♝ ♛ ♚ ♝ . ♜
 7  ♟︎ ♟︎ . ♟︎ . ♟︎ ♙ ♟︎
 6  . . . . . . . .
 5  . . ♟︎ . ♟︎ ♙ . .
 4  . ♙ . ♙ . . . .
 3  . . . ♙ . . . ♙
 2  ♙ . ♙ . ♙ . . .
 1  ♖ ♘ ♗ ♕ ♔ ♗ ♘ ♖

    a b c d e f g h

    Side:     white
    En passant:  e6
    Castling:  KQkq

)" },

  { TestFen::cmk_position,
    R"(
 8  ♜ . . ♛ . ♜ ♚ .
 7  ♟︎ ♟︎ ♟︎ . . ♟︎ ♟︎ ♟︎
 6  . . ♞ . ♝ ♞ . .
 5  . . ♝ . ♟︎ . . .
 4  . . . ♟︎ ♙ . . .
 3  . . . ♙ . ♘ ♙ ♙
 2  ♙ ♙ ♙ . ♘ ♙ ♗ .
 1  ♖ . ♗ ♕ . ♖ ♔ .

    a b c d e f g h

    Side:     black
    En passant:  no
    Castling:  ----

)" }
#endif
};

class game_test_fixture : public testing::Test {
  protected:
  void SetUp() override;

  public:
  Game game{ };
};

void game_test_fixture::SetUp() { }

TEST_F(game_test_fixture, fen_parsing_test) {
  for(const auto& [fen, board_str] : test_fens) {
    game.parse_fen(fen);
    EXPECT_STREQ(board_str.c_str(), game.print_board(false).c_str());
  }
}

TEST_F(game_test_fixture, fen_parsing_sets_occupancies) {
  game.parse_fen(Fen::start_position);

  const Board& board = GameTestAccess::board(game);
  u64 expected_white{ };
  for(Pieces piece{ P }; piece <= K; ++piece) {
    expected_white |= board.state.bitboards[piece];
  }

  u64 expected_black{ };
  for(Pieces piece{ p }; piece <= k; ++piece) {
    expected_black |= board.state.bitboards[piece];
  }

  EXPECT_EQ(board.state.occupancies[white], expected_white);
  EXPECT_EQ(board.state.occupancies[black], expected_black);
  EXPECT_EQ(board.state.occupancies[both], (expected_white | expected_black));
  EXPECT_NE(board.state.occupancies[white], board.state.occupancies[black]);
}

TEST_F(game_test_fixture, parse_fen_throws_on_invalid_input) {
  // Obviously incorrect FEN (too short, missing fields)
  EXPECT_THROW(game.parse_fen("invalid"), runtime_error);
}

TEST_F(game_test_fixture, parse_fen_rejects_malformed_fens_and_keeps_the_board) {
  game.parse_fen(Fen::start_position);
  const auto before = GameTestAccess::board(game).state;

  for(const char* bad : {
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1", // 7 ranks
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/8 w - - 0 1", // 9 ranks
          "rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", // 9 files
          "rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", // bad digit
          "rnbqkbnx/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", // bad piece
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x - - 0 1", // bad side
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQxq - 0 1", // bad castling
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - e9 0 1", // bad en passant
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w", // missing fields
      }) {
    EXPECT_THROW(game.parse_fen(bad), runtime_error) << bad;
  }

  EXPECT_EQ(GameTestAccess::board(game).state.bitboards, before.bitboards);
  EXPECT_EQ(GameTestAccess::board(game).state.side, before.side);
}

TEST_F(game_test_fixture, parse_fen_reads_all_state_fields) {
  game.parse_fen("rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR b Kq e6 12 30");
  const auto& state = GameTestAccess::board(game).state;
  EXPECT_EQ(state.side, black);
  EXPECT_EQ(state.castle, wk | bq);
  EXPECT_EQ(state.en_passant, e6);
  EXPECT_EQ(state.halfmove, 12);
}
