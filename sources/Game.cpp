#include "../headers/Game.h"
#include "../headers/Bitboard.h"
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>

using namespace std;

namespace maharajah {

Game::Game() {
  init_all();
}

GameState Game::state() const {
  return game_state_;
}

int Game::shutdown() {
  game_state_ = end_game;
  return score_;
}

string Game::print_bitboard(const u64 bitboard, const bool print_to_console) {
  stringstream ss;
  if(print_to_console) {
    ss << "0x" << hex << bitboard << ",";
  } else {
    for(int rank{}; rank < rank_bit; rank++) {
      for(int file{}; file < file_bit; file++) {
        const Squares square{ to_square(rank * rank_bit + file) };

        if(!file) {
          string rank_string = to_string(rank_bit - rank);
          ss << format(" {} ", rank_string);
        }
        ss << " ";
        const bool bit{ get_bit(bitboard, square) };
        ss << (bit ? "1" : "0");
      }
      ss << '\n';
    }
    ss << "\n    a b c d e f g h\n";
    ss << "    Bitboard: ";
    ss << "0x" << hex << bitboard << '\n';
  }

  if(print_to_console) {
    cout << ss.str() << '\n';
  }
  return ss.str();
}

string Game::print_board(const bool print_to_console) const {
  stringstream ss;
  ss << "\n";
  for(int rank{}; rank < rank_bit; rank++) {
    for(int file{}; file < file_bit; file++) {
      int square{ rank * rank_bit + file };
      if(!file)
        ss << " " << 8 - rank << " ";

      int piece_int{ -1 };

      for(Pieces piece{ P }; piece < no_pieces; piece++) {
        if(get_bit(board_.state.bitboards[piece], to_square(square)))
          piece_int = static_cast<int>(piece);
      }

      ss << format(" {}", (piece_int == -1) ? "." : display_pieces[piece_int]);
    }
    ss << "\n";
  }

  ss << "\n    a b c d e f g h\n\n";
  ss << format("    Side:     {}\n", board_.state.side ? "black" : "white");
  ss << format("    En passant:  {}\n",
               (board_.state.en_passant != no_square) ? square_to_coord[board_.state.en_passant] : "no");
  ss << format("    Castling:  {}{}{}{}\n",
               (board_.state.castle & wk) ? 'K' : '-',
               (board_.state.castle & wq) ? 'Q' : '-',
               (board_.state.castle & bk) ? 'k' : '-',
               (board_.state.castle & bq) ? 'q' : '-');
  ss << "\n";

  string str = ss.str();
  if(print_to_console)
    cout << str;

  return str;
}

void Game::parse_fen(const string_view fen) {
  board_.state.side = white;
  board_.state.en_passant = no_square;
  board_.state.castle = 0;
  int index = 0;
  const auto fen_size = static_cast<int>(fen.size());

  auto require_in_range = [fen_size](const int idx) {
    if(idx < 0 || idx >= fen_size) {
      throw runtime_error("Invalid FEN");
    }
  };

  auto fen_char = [&](int idx) -> char {
    require_in_range(idx);
    return fen[idx];
  };

  board_.state.bitboards.fill(zero);
  board_.state.occupancies.fill(zero);

  for(int rank{}; rank < rank_bit; rank++) {
    for(int file{}; file < file_bit; file++) {

      auto square = to_square(rank * rank_bit + file);
      const char c1 = fen_char(index);

      if((c1 >= 'a' && c1 <= 'z') || (c1 >= 'A' && c1 <= 'Z')) {
        int piece{ char_pieces[c1] };
        set_bit(board_.state.bitboards[piece], square);
        index++;
      }

      const char c2 = fen_char(index);
      if(c2 >= '0' && c2 <= '9') {
        int offset{ c2 - '0' };
        int piece_int{ -1 };

         for(Pieces piece{ P }; piece < no_pieces; piece++) {
          if(get_bit(board_.state.bitboards[piece], square))
            piece_int = static_cast<int>(piece);
        }

        if(piece_int == -1)
          file--;

        file += offset;
        index++;
      }

      if(fen_char(index) == '/')
        index++;
    }
  }

  index++;
  const char side_char = fen_char(index);
  (side_char == 'w') ? (board_.state.side = white) : (board_.state.side = black);
  index += 2;

  while(true) {
    const char c = fen_char(index);
    if(c == ' ')
      break;

    switch(c) {
    case 'K':
      board_.state.castle |= wk;
      break;
    case 'Q':
      board_.state.castle |= wq;
      break;
    case 'k':
      board_.state.castle |= bk;
      break;
    case 'q':
      board_.state.castle |= bq;
      break;
    case '-': default:
      break;
    }
    index++;
  }

  index++;

  if(fen_char(index) != '-') {
    const int file{ fen_char(index) - 'a' };
    ++index;
    const int rank{ rank_bit - (fen_char(index) - '0') };
    board_.state.en_passant = to_square(rank * rank_bit + file);
  } else {
    board_.state.en_passant = no_square;
  }

  for(Pieces piece{ P }; piece <= K; piece++)
    board_.state.occupancies[white] |= board_.state.bitboards[piece];

  for(Pieces piece{ p }; piece <= k; piece++)
    board_.state.occupancies[black] |= board_.state.bitboards[piece];

  board_.state.occupancies[both] |= board_.state.occupancies[white];
  board_.state.occupancies[both] |= board_.state.occupancies[black];
}

void Game::print_attacked_squares(Colors side) const {
  stringstream ss;

  for(int rank{}; rank < rank_bit; rank++) {
    for(int file{}; file < file_bit; file++) {
      const auto square = to_square(rank * rank_bit + file);
      if(!file)
        ss << " " << rank_bit - rank << " ";

      ss << " " << (board_.is_square_attacked(square, side) ? 1 : 0);
    }
    ss << "\n";
  }
  ss << ("\n    a b c d e f g h\n\n");
  cout << ss.str();
}

void Game::print_move(const int move) {
  const Pieces promoted = Move::get_move_promoted(move);
  const char promo_char = (promoted == no_pieces) ? ' ' : promoted_pieces[promoted];

  cout << format("{}{}{}\n",
                 square_to_coord[Move::get_move_source(move)],
                 square_to_coord[Move::get_move_target(move)],
                 promo_char);
}

void Game::print_move_list() {
  stringstream ss;

  if(board_.moves_list.size() == 0) {
    ss << "\n     No move in the move list!\n";
    return;
  }

  ss << "\n     move    piece     capture   double    enpass    castling\n\n";

  for(size_t move_count{}; move_count < board_.moves_list.size(); move_count++) {
    const int move = board_.moves_list[move_count];

    ss << format("      {}{}{}   {}         {}         {}         {}         {}\n",
                 square_to_coord[Move::get_move_source(move)],
                 square_to_coord[Move::get_move_target(move)],
                 Move::get_move_promoted(move) == no_pieces ? ' ' : promoted_pieces[Move::get_move_promoted(move)],
                 display_pieces[Move::get_move_piece(move)],
                 Move::get_move_capture(move) ? 1 : 0,
                 Move::get_move_double(move) ? 1 : 0,
                 Move::get_move_enpassant(move) ? 1 : 0,
                 Move::get_move_castling(move) ? 1 : 0);
  }

  ss << format("\n\n     Total number of moves: {}\n\n", board_.moves_list.size());

  cout << ss.str();
}

void Game::play() {
  game_state_ = play_game;

  parse_fen(killer_position);
  string str = print_board();
}

} // namespace maharajah
