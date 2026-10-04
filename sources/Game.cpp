#include "../headers/Game.h"
#include "../headers/Bitboard.h"
#include "../headers/Clock.h"
#include "../headers/Move.h"
#include "../headers/Notation.h"
#include "../headers/Perft.h"
#include "../headers/Search.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <sys/select.h>
#  include <unistd.h>
#endif

// CMake defines MAHARAJAH_VERSION as a string literal. The Flutter build hook
// can't: on Windows it runs cl.exe through cmd.exe, which mangles the command
// line when an argument carries quotes. It passes the bare MAHARAJAH_VERSION_NUMBER
// instead, and the string is made here.
#if !defined(MAHARAJAH_VERSION) && defined(MAHARAJAH_VERSION_NUMBER)
#  define MAHARAJAH_STRINGIFY_(x) #x
#  define MAHARAJAH_STRINGIFY(x) MAHARAJAH_STRINGIFY_(x)
#  define MAHARAJAH_VERSION MAHARAJAH_STRINGIFY(MAHARAJAH_VERSION_NUMBER)
#endif

using namespace std;
using namespace maharajah;

namespace maharajah {

namespace {

// the console input buffer, before any redirection (tests swap std::cin's buffer)
streambuf* const console_input = cin.rdbuf();

// true when a line of input is waiting, so a running search can react to "stop"
bool input_waiting() {
  streambuf* const input = cin.rdbuf();
  if(input->in_avail() > 0)
    return true;
  if(input != console_input)
    return false;

#if defined(_WIN32)
  static const HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
  static DWORD mode{ };
  static const bool pipe = !GetConsoleMode(handle, &mode);
  DWORD available{ };
  if(pipe)
    return !PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr) || available > 0;
  GetNumberOfConsoleInputEvents(handle, &available);
  return available > 1;
#else
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(STDIN_FILENO, &read_fds);
  timeval timeout{ };
  select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
  return FD_ISSET(STDIN_FILENO, &read_fds);
#endif
}

// Any input stops a running search; "quit" also ends the UCI loop.
void poll_input(TimeControl& time) {
  if(!input_waiting())
    return;

  time.stopped = true;

  string line;
  if(!getline(cin, line) || line.starts_with("quit"))
    time.quit = true;
}

// integer following `name` in a "go" command, or `fallback` when absent
int go_argument(const string_view command, const string_view name, const int fallback) {
  const auto position = command.find(name);
  if(position == string_view::npos)
    return fallback;
  return atoi(string(command.substr(position + name.size())).c_str());
}

} // namespace

Game::Game() {
  engine_.time_control.poll = poll_input;
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
          ss << " " << rank_string << " ";
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
        if(get_bit(engine_.board.state.bitboards[piece], to_square(square)))
          piece_int = static_cast<int>(piece);
      }

      ss << " ";
      if(piece_int == -1)
        ss << ".";
      else
        ss << Notation::display_pieces[piece_int];
    }
    ss << "\n";
  }

  ss << "\n    a b c d e f g h\n\n";
  ss << "    Side:     " << (engine_.board.state.side ? "black" : "white") << "\n";
  ss << "    En passant:  ";
  if(engine_.board.state.en_passant != no_square)
    ss << Notation::square_to_coordinates[engine_.board.state.en_passant];
  else
    ss << "no";
  ss << "\n";
  ss << "    Castling:  "
     << ((engine_.board.state.castle & wk) ? 'K' : '-')
     << ((engine_.board.state.castle & wq) ? 'Q' : '-')
     << ((engine_.board.state.castle & bk) ? 'k' : '-')
     << ((engine_.board.state.castle & bq) ? 'q' : '-') << "\n";
  ss << "\n";

  string str = ss.str();
  if(print_to_console)
    cout << str;

  return str;
}

void Game::parse_fen(const string_view fen) {
  engine_.board.parse_fen(fen);
}

void Game::print_attacked_squares(Colors side) const {
  stringstream ss;

  for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
    for(int file{ }; file < BoardGeometry::files; ++file) {
      const auto square = to_square(rank * BoardGeometry::ranks + file);
      if(!file)
        ss << " " << BoardGeometry::ranks - rank << " ";

      ss << " " << (engine_.board.is_square_attacked(square, side) ? 1 : 0);
    }
    ss << "\n";
  }
  ss << ("\n    a b c d e f g h\n\n");
  cout << ss.str();
}

void Game::print_move(const int move) {
  cout << Board::move_to_string(move) << '\n';
}

void Game::print_move_list() {
  stringstream ss;

  if(engine_.board.moves_list.size() == 0) {
    ss << "\n     No move in the move list!\n";
    cout << ss.str();
    return;
  }

  ss << "\n     move    piece     capture   double    enpass    castling\n\n";

  for(size_t move_count{ }; move_count < engine_.board.moves_list.size(); ++move_count) {
    const int move = engine_.board.moves_list[move_count];

    ss << "      " << left << setw(5) << Board::move_to_string(move) << right
       << "   " << Notation::display_pieces[Move::get_move_piece(move)]
       << "         " << (Move::get_move_capture(move) ? 1 : 0)
       << "         " << (Move::get_move_double(move) ? 1 : 0)
       << "         " << (Move::get_move_enpassant(move) ? 1 : 0)
       << "         " << (Move::get_move_castling(move) ? 1 : 0) << "\n";
  }

  ss << "\n\n     Total number of moves: " << engine_.board.moves_list.size() << "\n\n";

  cout << ss.str();
}

// parse user/GUI move string input (e.g. "e7e8q")
int Game::parse_move(const string_view move_string) const {
  return engine_.board.parse_move(move_string);
}

// search position for the best move
void Game::search_position(const int depth) {
  const SearchResult result = run_search(engine_, depth, &cout);

  cout << "bestmove " << (result.best_move ? Board::move_to_string(result.best_move) : "(none)") << '\n' << flush;
}

// parse UCI "go" command, e.g. "go depth 8", "go movetime 1000", "go wtime 60000 btime 60000 winc 1000"
void Game::parse_go(const string_view command) {
  // "go perft N": legal move paths to depth N, per root move as in Stockfish
  if(command.find("perft") != string_view::npos) {
    const u64 nodes = Perft(engine_.board).perft_divide(go_argument(command, "perft", 1));
    cout << "\nNodes searched: " << nodes << "\n\n" << flush;
    return;
  }

  TimeControl& time = engine_.time_control;
  time.reset();

  const Colors side = engine_.board.state.side;
  const int increment = go_argument(command, side == white ? "winc" : "binc", 0);
  int uci_time = go_argument(command, side == white ? "wtime" : "btime", -1);
  const int clock_time = uci_time;
  int moves_to_go = go_argument(command, "movestogo", 30);
  const int movetime = go_argument(command, "movetime", -1);
  int depth = go_argument(command, "depth", -1);

  if(movetime != -1) {
    uci_time = movetime;
    moves_to_go = 1;
  }

  time.starttime = now_ms();

  if(uci_time != -1) {
    time.timeset = true;

    if(moves_to_go > 0)
      uci_time /= moves_to_go;

    // leave a margin for GUI lag
    if(uci_time > 1500)
      uci_time -= 50;

    time.stoptime = time.starttime + uci_time + increment;

    if(uci_time < 1500 && increment && depth == Limits::max_ply) {
      // with a tiny increment the lag margin could place stoptime in the past
      // and abort the search before depth 1 completes
      time.stoptime = time.starttime + max(increment - 50, 5);
    }

    // On a clock the share of the remaining time above is a target: no iteration
    // starts after 60 % of it, and one already running may go on to 2.5 times it,
    // but not past 40 % of the time left.
    if(movetime == -1 && clock_time > 0) {
      const int target = time.stoptime - time.starttime;
      time.soft_stoptime = time.starttime + target * 3 / 5;
      time.stoptime = time.starttime + max(target, min(target * 5 / 2, clock_time * 2 / 5));
    }
  }

  // no depth limit: search until the time runs out or "stop" arrives
  if(depth == -1)
    depth = Limits::max_ply;

  // a pure fixed-depth search runs to completion; anything else listens for "stop"
  time.poll_input = time.timeset || movetime != -1 || depth == Limits::max_ply;

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
void Game::parse_position(string_view command) {
  if(command.starts_with("position"))
    command.remove_prefix(8);

  const auto moves_position = command.find("moves");
  const string_view setup = command.substr(0, moves_position);

  try {
    bool valid = true;

    if(const auto fen_position = setup.find("fen"); fen_position != string_view::npos && setup.substr(0, fen_position).find("startpos") == string_view::npos)
      valid = engine_.set_position(setup.substr(fen_position + 3));
    else
      engine_.set_position(Fen::start_position);

    if(!valid) {
      cout << "info string invalid position: each side needs exactly one king\n" << flush;
      return;
    }
  } catch(const runtime_error&) {
    cout << "info string invalid fen\n" << flush;
    return;
  }

  if(moves_position != string_view::npos) {
    istringstream moves{ string(command.substr(moves_position + 5)) };
    string move;

    while(moves >> move) {
      if(!engine_.apply_move(move)) {
        cout << "info string illegal move ignored\n" << flush;
        break;
      }
    }
  }

  if(verbose_)
    print_board();
}

void Game::print_uci_info() {
  cout << "id name Maharajah " << MAHARAJAH_VERSION << "\n";
  cout << "id author Villi\n";
  cout << "option name Hash type spin default " << default_hash_mb << " min " << min_hash_mb << " max " << max_hash_mb << "\n";
  cout << "option name Skill Level type spin default " << SearchConfig::max_skill << " min " << SearchConfig::min_skill << " max " << SearchConfig::max_skill << "\n";
  cout << "option name Threads type spin default " << Engine::min_threads << " min " << Engine::min_threads << " max " << Engine::max_threads << "\n";
  cout << "option name EvalFile type string default <empty>\n";
  cout << "option name UseNNUE type check default true\n";
  cout << "uciok\n" << flush;
}

void Game::uci_loop() {
  string input;

  print_uci_info();

  while(getline(cin, input)) {
    if(!input.empty() && input.back() == '\r')
      input.pop_back();

    if(input.empty())
      continue;

    // isready
    if(starts_with(input, "isready")) {
      cout << "readyok\n" << flush;
    }

    // position (the transposition table is kept between the moves of a game)
    else if(starts_with(input, "position")) {
      parse_position(input);
    }

    // ucinewgame
    else if(starts_with(input, "ucinewgame")) {
      parse_position("position startpos");
      engine_.transposition_table.clear();
    }

    // go
    else if(is_token(input, "go")) {
      parse_go(input);
    }

    // quit
    else if(starts_with(input, "quit")) {
      break;
    }

    // uci
    else if(starts_with(input, "uci")) {
      print_uci_info();
    }

    else if(starts_with(input, "setoption name Hash value ")) {
      const int mb = clamp(atoi(input.c_str() + 26), min_hash_mb, max_hash_mb);
      engine_.transposition_table.resize(mb);
    }

    else if(starts_with(input, "setoption name Skill Level value ")) {
      engine_.search_config = SearchConfig::for_skill(atoi(input.c_str() + 33));
    }

    else if(starts_with(input, "setoption name Threads value ")) {
      engine_.threads = clamp(atoi(input.c_str() + 29), Engine::min_threads, Engine::max_threads);
    }

    // a network file replaces the built-in network, "<empty>" restores it; a file that
    // cannot be loaded leaves the classic evaluation
    else if(starts_with(input, "setoption name EvalFile value ")) {
      const string path = input.substr(30);
      if(path.empty() || path == "<empty>") {
        engine_.nnue.load_default_weights();
      } else if(!engine_.nnue.load_weights(path.c_str())) {
        cout << "info string cannot load the network " << path << "\n" << flush;
      }
    }

    // "false" switches to the classic evaluation
    else if(starts_with(input, "setoption name UseNNUE value ")) {
      engine_.eval_config.eval_mode = input.substr(29) == "false" ? EvalMode::classic : EvalMode::nnue;
    }

    // "quit" arrived during a search
    if(engine_.time_control.quit)
      break;
  }
}

void Game::play(const bool debug) {
  game_state_ = play_game;
  verbose_ = debug;

  if(debug) {
    parse_fen(Fen::start_position);
    print_board();
    search_position(6);
  } else
    uci_loop();
}

} // namespace maharajah
