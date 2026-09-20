#include "../headers/Game.h"
#include "../headers/Bitboard.h"
#include "../headers/Notation.h"
#include "../headers/Perft.h"
#include "../headers/Search.h"
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std;
using namespace maharajah;

namespace maharajah {

Game::Game() {
  AttackTables::init();
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
    for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
      for(int file{ }; file < BoardGeometry::files; ++file) {
        const Squares square{ to_square(rank * BoardGeometry::ranks + file) };

        if(!file) {
          string rank_string = to_string(BoardGeometry::ranks - rank);
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
  for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
    for(int file{ }; file < BoardGeometry::files; ++file) {
      int square{ rank * BoardGeometry::ranks + file };
      if(!file)
        ss << " " << 8 - rank << " ";

      int piece_int{ -1 };

      for(Pieces piece{ P }; piece < no_pieces; ++piece) {
        if(get_bit(board_.state.bitboards[piece], to_square(square)))
          piece_int = static_cast<int>(piece);
      }

      ss << format(" {}", (piece_int == -1) ? "." : Notation::display_pieces[piece_int]);
    }
    ss << "\n";
  }

  ss << "\n    a b c d e f g h\n\n";
  ss << format("    Side:     {}\n", board_.state.side ? "black" : "white");
  ss << format("    En passant:  {}\n", (board_.state.en_passant != no_square) ? Notation::square_to_coordinates[board_.state.en_passant] : "no");
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
  const auto invalid = [] { return runtime_error("Invalid FEN"); };

  string placement, side_field, castle_field, en_passant_field, halfmove_field;
  istringstream fields{ string(fen) };
  if(!(fields >> placement >> side_field >> castle_field >> en_passant_field))
    throw invalid();
  fields >> halfmove_field; // optional, followed by the optional fullmove number

  // parse into a temporary so a malformed FEN leaves the board untouched
  BoardState parsed{ };

  constexpr string_view piece_chars{ "PNBRQKpnbrqk" };
  int rank{ }, file{ };
  for(const char ch : placement) {
    if(ch == '/') {
      if(file != BoardGeometry::files || ++rank >= BoardGeometry::ranks)
        throw invalid();
      file = 0;
    } else if(ch >= '1' && ch <= '8') {
      file += ch - '0';
      if(file > BoardGeometry::files)
        throw invalid();
    } else {
      const auto piece = piece_chars.find(ch);
      if(piece == string_view::npos || file >= BoardGeometry::files)
        throw invalid();
      set_bit(parsed.bitboards[piece], to_square(rank * BoardGeometry::ranks + file));
      ++file;
    }
  }
  if(rank != BoardGeometry::ranks - 1 || file != BoardGeometry::files)
    throw invalid();

  if(side_field == "w")
    parsed.side = white;
  else if(side_field == "b")
    parsed.side = black;
  else
    throw invalid();

  if(castle_field != "-") {
    for(const char ch : castle_field) {
      switch(ch) {
      case 'K':
        parsed.castle |= wk;
        break;
      case 'Q':
        parsed.castle |= wq;
        break;
      case 'k':
        parsed.castle |= bk;
        break;
      case 'q':
        parsed.castle |= bq;
        break;
      default:
        throw invalid();
      }
    }
  }

  if(en_passant_field != "-") {
    if(en_passant_field.size() != 2 || en_passant_field[0] < 'a' || en_passant_field[0] > 'h' || en_passant_field[1] < '1' || en_passant_field[1] > '8')
      throw invalid();
    parsed.en_passant = to_square((BoardGeometry::ranks - (en_passant_field[1] - '0')) * BoardGeometry::ranks + (en_passant_field[0] - 'a'));
  }

  if(!halfmove_field.empty())
    from_chars(halfmove_field.data(), halfmove_field.data() + halfmove_field.size(), parsed.halfmove);

  for(Pieces piece{ P }; piece <= K; ++piece)
    parsed.occupancies[white] |= parsed.bitboards[piece];

  for(Pieces piece{ p }; piece <= k; ++piece)
    parsed.occupancies[black] |= parsed.bitboards[piece];

  parsed.occupancies[both] = parsed.occupancies[white] | parsed.occupancies[black];

  board_.state = parsed;
  board_.ply = 0; // Reset ply on new game
}

void Game::print_attacked_squares(Colors side) const {
  stringstream ss;

  for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
    for(int file{ }; file < BoardGeometry::files; ++file) {
      const auto square = to_square(rank * BoardGeometry::ranks + file);
      if(!file)
        ss << " " << BoardGeometry::ranks - rank << " ";

      ss << " " << (board_.is_square_attacked(square, side) ? 1 : 0);
    }
    ss << "\n";
  }
  ss << ("\n    a b c d e f g h\n\n");
  cout << ss.str();
}

void Game::print_move(const int move) {
  const Pieces promoted = Move::get_move_promoted(move);
  const char promo_char = (promoted == no_pieces) ? ' ' : Notation::promoted_pieces[promoted];

  cout << format(
      "{}{}{}\n", Notation::square_to_coordinates[Move::get_move_source(move)], Notation::square_to_coordinates[Move::get_move_target(move)], promo_char);
}

void Game::print_move_list() {
  stringstream ss;

  if(board_.moves_list.size() == 0) {
    ss << "\n     No move in the move list!\n";
    cout << ss.str();
    return;
  }

  ss << "\n     move    piece     capture   double    enpass    castling\n\n";

  for(size_t move_count{ }; move_count < board_.moves_list.size(); ++move_count) {
    const int move = board_.moves_list[move_count];

    ss << format("      {}{}{}   {}         {}         {}         {}         {}\n",
                 Notation::square_to_coordinates[Move::get_move_source(move)],
                 Notation::square_to_coordinates[Move::get_move_target(move)],
                 Move::get_move_promoted(move) == no_pieces ? ' ' : Notation::promoted_pieces[Move::get_move_promoted(move)],
                 Notation::display_pieces[Move::get_move_piece(move)],
                 Move::get_move_capture(move) ? 1 : 0,
                 Move::get_move_double(move) ? 1 : 0,
                 Move::get_move_enpassant(move) ? 1 : 0,
                 Move::get_move_castling(move) ? 1 : 0);
  }

  ss << format("\n\n     Total number of moves: {}\n\n", board_.moves_list.size());

  cout << ss.str();
}

// parse user/GUI move string input (e.g. "e7e8q")
int Game::parse_move(const char* move_string) {
  if(!move_string) {
    return 0;
  }

  const size_t move_length = std::strlen(move_string);
  if(move_length < 4) {
    return 0;
  }

  MoveList move_list;
  board_.generate_moves(move_list);

  // parse source square
  int source_square = (move_string[0] - 'a') + (BoardGeometry::ranks - (move_string[1] - '0')) * BoardGeometry::ranks;

  // parse target square
  int target_square = (move_string[2] - 'a') + (BoardGeometry::ranks - (move_string[3] - '0')) * BoardGeometry::ranks;

  // loop over the moves within a move list
  for(size_t i{ }; i < move_list.size(); ++i) {
    // init move
    int move = move_list[i];

    // make sure source & target squares are available within the generated move
    if(source_square == Move::get_move_source(move) && target_square == Move::get_move_target(move)) {
      // init promoted piece
      int promoted_piece = Move::get_move_promoted(move);

      // promoted piece is available
      if(promoted_piece != no_pieces) {
        if(move_length < 5) {
          return 0;
        }
        // promoted to queen
        if((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q')
          // return legal move
          return move;

        // promoted to rook
        else if((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r')
          // return legal move
          return move;

        // promoted to bishop
        else if((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b')
          // return legal move
          return move;

        // promoted to knight
        else if((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n')
          // return legal move
          return move;

        // continue the loop on possible wrong promotions (e.g. "e7e8f")
        continue;
      }

      // return legal move
      return move;
    }
  }

  // return illegal move
  return 0;
}

// search position for the best move
void Game::search_position(const int depth) {
  Search search(board_);
  const SearchResult result = search.run(depth);

  if(result.best_move) {
    cout << format("info score cp {} depth {} nodes {}\n", result.score, result.depth, result.nodes);
    cout << "bestmove ";
    print_move(result.best_move);
    cout << flush;
  }
}

// parse UCI "go" command
void Game::parse_go(const char* command) {
  int depth = 6; // default

  const char* ptr = std::strstr(command, "depth");
  if(ptr) {
    ptr += strlen("depth"); // +6

    // skip ' '
    while(*ptr == ' ')
      ptr++;

    if(*ptr >= '0' && *ptr <= '9') {
      depth = atoi(ptr);
    }
  }

  search_position(depth);
}

/*  Example UCI commands to init position on chess board

    // init start position
    position startpos

    // init start position and make the moves on chess board
    position startpos moves e2e4 e7e5

    // init position from FEN string
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1

    // init position from fen string and make moves on chess board
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 moves e2a6 e8g8
*/
void Game::parse_position(char* command) {
  char* ptr = command;

  if(strncmp(ptr, "position", 8) == 0) {
    ptr += 8;
  }

  while(*ptr == ' ')
    ptr++;

  try {
    if(strncmp(ptr, "startpos", 8) == 0) {
      parse_fen(Fen::start_position);
      ptr += 8;
    }

    else if(strncmp(ptr, "fen", 3) == 0) {
      ptr += 3;

      while(*ptr == ' ')
        ptr++;

      parse_fen(ptr);
    } else {
      parse_fen(Fen::start_position);
    }
  } catch(const runtime_error&) {
    cout << "info string invalid fen\n";
    return;
  }

  if(count_bits(board_.state.bitboards[K]) != 1 || count_bits(board_.state.bitboards[k]) != 1) {
    cout << "info string invalid position: each side needs exactly one king\n";
    parse_fen(Fen::start_position);
    return;
  }

  char* moves_ptr = strstr(ptr, "moves");

  if(moves_ptr) {
    ptr = moves_ptr + 5;

    while(*ptr == ' ')
      ptr++;

    while(*ptr) {
      int move = parse_move(ptr);

      if(move == 0)
        break;

      if(!board_.make_move(move, TypeMove::all_moves)) {
        cout << "info string illegal move ignored\n";
        break;
      }

      while(*ptr && *ptr != ' ')
        ptr++;

      while(*ptr == ' ')
        ptr++;
    }
  }

  if(verbose_)
    print_board();
}

void Game::uci_loop() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  char input[2000];

  print_uci_info();

  while(true) {
    if(!std::cin.getline(input, sizeof(input)))
      break;

    if(input[0] == '\0')
      continue;

    // isready
    if(starts_with(input, "isready")) {
      std::cout << "readyok\n";
    }

    // position
    else if(starts_with(input, "position")) {
      parse_position(input);
    }

    // ucinewgame
    else if(starts_with(input, "ucinewgame")) {
      char start[] = "position startpos";
      parse_position(start);
    }

    // go
    else if(is_token(input, "go")) {
      parse_go(input);
    }

    // stop
    else if(starts_with(input, "stop")) {
      // TODO: search_stop = true;
    }

    // quit
    else if(starts_with(input, "quit")) {
      break;
    }

    // uci
    else if(starts_with(input, "uci")) {
      print_uci_info();
    }
  }
}

void Game::play(const bool debug) {
  game_state_ = play_game;
  AttackTables::init();
  verbose_ = debug;

  if(debug) {
    parse_fen(Fen::start_position);
    print_board();
    search_position(6);
  } else
    uci_loop();
}

} // namespace maharajah
