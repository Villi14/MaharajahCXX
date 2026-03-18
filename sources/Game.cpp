#include "../headers/Game.h"
#include "../headers/Bitboard.h"
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#  include <windows.h>
#else
#  include <sys/time.h>
#endif

using namespace std;
using namespace maharajah;

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
    for(int rank{}; rank < rank_bit; ++rank) {
      for(int file{}; file < file_bit; ++file) {
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
  for(int rank{}; rank < rank_bit; ++rank) {
    for(int file{}; file < file_bit; ++file) {
      int square{ rank * rank_bit + file };
      if(!file)
        ss << " " << 8 - rank << " ";

      int piece_int{ -1 };

      for(Pieces piece{ P }; piece < no_pieces; ++piece) {
        if(get_bit(board_.state.bitboards[piece], to_square(square)))
          piece_int = static_cast<int>(piece);
      }

      ss << format(" {}", (piece_int == -1) ? "." : display_pieces[piece_int]);
    }
    ss << "\n";
  }

  ss << "\n    a b c d e f g h\n\n";
  ss << format("    Side:     {}\n", board_.state.side ? "black" : "white");
  ss << format("    En passant:  {}\n", (board_.state.en_passant != no_square) ? square_to_coordinates[board_.state.en_passant] : "no");
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

  for(int rank{}; rank < rank_bit; ++rank) {
    for(int file{}; file < file_bit; ++file) {

      auto square = to_square(rank * rank_bit + file);
      const char ch1 = fen_char(index);

      if((ch1 >= 'a' && ch1 <= 'z') || (ch1 >= 'A' && ch1 <= 'Z')) {
        int piece{ char_pieces[ch1] };
        set_bit(board_.state.bitboards[piece], square);
        ++index;
      }

      const char ch2 = fen_char(index);
      if(ch2 >= '0' && ch2 <= '9') {
        int offset{ ch2 - '0' };
        int piece_int{ -1 };

        for(Pieces piece{ P }; piece < no_pieces; ++piece) {
          if(get_bit(board_.state.bitboards[piece], square))
            piece_int = static_cast<int>(piece);
        }

        if(piece_int == -1)
          --file;

        file += offset;
        ++index;
      }

      if(fen_char(index) == '/')
        ++index;
    }
  }

  ++index;
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
    case '-':
    default:
      break;
    }
    ++index;
  }

  ++index;

  if(fen_char(index) != '-') {
    const int file{ fen_char(index) - 'a' };
    ++index;
    const int rank{ rank_bit - (fen_char(index) - '0') };
    board_.state.en_passant = to_square(rank * rank_bit + file);
  } else {
    board_.state.en_passant = no_square;
  }

  for(Pieces piece{ P }; piece <= K; ++piece)
    board_.state.occupancies[white] |= board_.state.bitboards[piece];

  for(Pieces piece{ p }; piece <= k; ++piece)
    board_.state.occupancies[black] |= board_.state.bitboards[piece];

  board_.state.occupancies[both] |= board_.state.occupancies[white];
  board_.state.occupancies[both] |= board_.state.occupancies[black];
}

void Game::print_attacked_squares(Colors side) const {
  stringstream ss;

  for(int rank{}; rank < rank_bit; ++rank) {
    for(int file{}; file < file_bit; ++file) {
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
  if(Move::get_move_promoted(move)) {
    cout << format("{}{}{}\n",
                   square_to_coordinates[Move::get_move_source(move)],
                   square_to_coordinates[Move::get_move_target(move)],
                   promoted_pieces[Move::get_move_promoted(move)]);

  } else {
    cout << format("{}{}\n", square_to_coordinates[Move::get_move_source(move)], square_to_coordinates[Move::get_move_target(move)]);
  }

  const Pieces promoted = Move::get_move_promoted(move);
  const char promo_char = (promoted == no_pieces) ? ' ' : promoted_pieces[promoted];

  cout << format("{}{}{}\n", square_to_coordinates[Move::get_move_source(move)], square_to_coordinates[Move::get_move_target(move)], promo_char);
}

void Game::print_move_list() {
  stringstream ss;

  if(board_.moves_list.size() == 0) {
    ss << "\n     No move in the move list!\n";
    return;
  }

  ss << "\n     move    piece     capture   double    enpass    castling\n\n";

  for(size_t move_count{}; move_count < board_.moves_list.size(); ++move_count) {
    const int move = board_.moves_list[move_count];

    ss << format("      {}{}{}   {}         {}         {}         {}         {}\n",
                 square_to_coordinates[Move::get_move_source(move)],
                 square_to_coordinates[Move::get_move_target(move)],
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

int Game::get_time_ms() {
#ifdef _MSC_VER
  return GetTickCount();
#else
  struct timeval time_value{};
  gettimeofday(&time_value, nullptr);
  return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
#endif
}

void Game::perft_driver(int depth) {
  if(depth == 0) {
    nodes_++;
    return;
  }

  board_.generate_moves();
  const MoveList moves = board_.moves_list;

  for(size_t i{}; i < moves.size(); ++i) {
    if(!board_.make_move(moves[i], TypeMove::all_moves)) {
      continue;
    }

    perft_driver(depth - 1);
    board_.pop_state();
  }
}

u64 Game::perft(int depth) {
  nodes_ = 0;
  perft_driver(depth);
  return static_cast<u64>(nodes_);
}

void Game::perft_divide(int depth) {
  board_.generate_moves();
  const MoveList moves_list = board_.moves_list;

  for(size_t i{}; i < moves_list.size(); ++i) {
    const int move = moves_list[i];

    if(!board_.make_move(move, TypeMove::all_moves)) {
      continue;
    }

    nodes_ = 0;
    perft_driver(depth - 1);

    board_.pop_state();

    cout << square_to_coordinates[Move::get_move_source(move)] << square_to_coordinates[Move::get_move_target(move)]
         << (Move::get_move_promoted(move) == no_pieces ? ' ' : promoted_pieces[Move::get_move_promoted(move)]) << ": " << nodes_ << '\n';
  }
}

void Game::perft_test(int depth) {
  cout << "\n      Performance test\n\n";

  board_.generate_moves();
  const MoveList moves_list = board_.moves_list;

  long start = get_time_ms();

  for(size_t i{}; i < moves_list.size(); ++i) {
    const int move = moves_list[i];

    if(!board_.make_move(move, TypeMove::all_moves)) {
      continue;
    }

    long count_nodes = nodes_;
    perft_driver(depth - 1);

    long old_nodes = nodes_ - count_nodes;
    board_.pop_state();

    cout << format("      move: {}{}{}    nodes: {}\n",
                   square_to_coordinates[Move::get_move_source(move)],
                   square_to_coordinates[Move::get_move_target(move)],
                   Move::get_move_promoted(move) ? promoted_pieces[Move::get_move_promoted(move)] : ' ',
                   old_nodes);
  }

  cout << "\n    Depth: " << depth;
  cout << "\n    Nodes: " << nodes_ << endl;
  cout << "\n     Time: " << get_time_ms() - start << endl;
}

// parse user/GUI move string input (e.g. "e7e8q")
int Game::parse_move(const char* move_string) {
  board_.generate_moves();
  const MoveList move_list = board_.moves_list;

  // parse source square
  int source_square = (move_string[0] - 'a') + (rank_bit - (move_string[1] - '0')) * rank_bit;

  // parse target square
  int target_square = (move_string[2] - 'a') + (rank_bit - (move_string[3] - '0')) * rank_bit;

  // loop over the moves within a move list
  for(size_t i{}; i < move_list.size(); ++i) {
    // init move
    int move = move_list[i];

    // make sure source & target squares are available within the generated move
    if(source_square == Move::get_move_source(move) && target_square == Move::get_move_target(move)) {
      // init promoted piece
      int promoted_piece = Move::get_move_promoted(move);

      // promoted piece is available
      if(promoted_piece != no_pieces) {
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
void Game::search_position(int depth) {
  printf("bestmove d2d4\n");
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

  cout << "depth: " << depth << endl;
}

/*
    Example UCI commands to init position on chess board

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

  if(strncmp(ptr, "startpos", 8) == 0) {
    parse_fen(start_position);
    ptr += 8;
  }

  else if(strncmp(ptr, "fen", 3) == 0) {
    ptr += 3;

    while(*ptr == ' ')
      ptr++;

    parse_fen(ptr);
  } else {
    parse_fen(start_position);
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

      board_.make_move(move, TypeMove::all_moves);

      while(*ptr && *ptr != ' ')
        ptr++;

      while(*ptr == ' ')
        ptr++;
    }
  }

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

void Game::play() {
  game_state_ = play_game;
  init_all();

  bool debug = true;

  if(debug) {
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");
    print_board();
  } else
    uci_loop();
}

} // namespace maharajah
