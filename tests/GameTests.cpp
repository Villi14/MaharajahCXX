#include "../headers/Game.h"
#include "gtest/gtest.h"

#include <map>

using namespace std;
using namespace maharajah;

map<string_view, string> test_fens = {

#ifdef _MSC_VER
  { empty_board,
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

  { start_position,
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

  { tricky_position,
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

  { killer_position,
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

  { cmk_position,
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
  { empty_board,
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

  { start_position,
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

  { tricky_position,
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

  { killer_position,
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

  { cmk_position,
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
  Game game{};
};

void game_test_fixture::SetUp() { }

/**
 * @brief Unit test for parsing FEN strings and comparing with the expected board state.
 */
TEST_F(game_test_fixture, fen_parsing_test) {
  for(const auto& [fen, board_str] : test_fens) {
    game.parse_fen(fen);
    EXPECT_STREQ(board_str.c_str(), game.print_board(false).c_str());
  }
}

TEST_F(game_test_fixture, parse_fen_throws_on_invalid_input) {
  // Obviously incorrect FEN (too short, missing fields)
  EXPECT_THROW(game.parse_fen("invalid"), runtime_error);
}
