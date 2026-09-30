// Native CLI over the C interface (mah_*): custom-position generation, self-play,
// a small benchmark, legal-move listing and a UCI-subset REPL. Port of
// maharajah_tool.c from MaharajahC; commands and output are the same.

#include "../headers/Clock.h"
#include "../headers/CustomSetup.h"
#include "../headers/ffi/MaharajahFfiInternal.h"
#include "../headers/ffi/maharajah_ffi.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace maharajah;

namespace {

constexpr std::size_t move_buffer_size{ 16 };
constexpr std::size_t fen_buffer_size{ 256 };

Board& live_board() {
  return ffi::live_engine().board;
}

void fail(const char* message) {
  std::fprintf(stderr, "maharajah_tool: %s\n", message);
}

// Print, one per line, every legal move for the side to move in the live position.
void print_legal_moves() {
  Board& board = live_board();
  MoveList list;
  board.generate_moves(list);

  for(std::size_t i{ }; i < list.size(); ++i) {
    if(!board.make_move(list[i], TypeMove::all_moves))
      continue;
    board.pop_state();
    std::printf("%s\n", Board::move_to_string(list[i]).c_str());
  }
}

// Every legal move of the position described by `fen`: the authoritative legal-move
// set that Maharajah_lab diffs against Fairy-Stockfish's.
int run_legalmoves_command(const char* fen) {
  if(!mah_init()) {
    fail("failed to initialize engine.");
    return 1;
  }

  // field 7 (per-side variant rights) is authoritative here, so the rules profile
  // is detected from the FEN rather than forced
  if(!mah_set_position_fen(fen)) {
    fail("failed to load FEN.");
    mah_shutdown();
    return 1;
  }

  print_legal_moves();

  mah_shutdown();
  return 0;
}

void json_write_string(std::FILE* out, const std::string_view value) {
  std::fputc('"', out);
  for(const char c : value) {
    if(c == '\\' || c == '"')
      std::fputc('\\', out);
    std::fputc(c, out);
  }
  std::fputc('"', out);
}

void json_write_game(std::FILE* out,
                     const bool first_game,
                     const int game_number,
                     const std::string_view start_fen,
                     const std::string_view result,
                     const std::string_view end_fen,
                     const std::vector<std::string>& moves) {
  if(!first_game)
    std::fprintf(out, ",\n");

  std::fprintf(out, "  {\n    \"game\": %d,\n    \"start_fen\": ", game_number);
  json_write_string(out, start_fen);
  std::fprintf(out, ",\n    \"result\": ");
  json_write_string(out, result);
  std::fprintf(out, ",\n    \"end_fen\": ");
  json_write_string(out, end_fen);
  std::fprintf(out, ",\n    \"moves\": [");

  for(std::size_t i{ }; i < moves.size(); ++i) {
    if(i > 0)
      std::fprintf(out, ", ");
    json_write_string(out, moves[i]);
  }

  std::fprintf(out, "]\n  }");
}

// std::rand is reseeded by the generator for every position, so the side to move of
// position i+1 comes from the stream seeded with seed+i, as in MaharajahC.
int run_generate_command(const int count, const unsigned int seed) {
  std::array<char, fen_buffer_size> fen{ };

  std::srand(seed);

  if(!mah_init()) {
    fail("failed to initialize engine.");
    return 1;
  }

  for(int index{ }; index < count; ++index) {
    const int side_to_move = std::rand() % 2;
    if(!mah_generate_custom_position_fen(side_to_move, seed + static_cast<unsigned int>(index), fen.data(), static_cast<int>(fen.size()))) {
      fail("failed to generate a valid custom position.");
      mah_shutdown();
      return 1;
    }

    std::printf("%s\n", fen.data());
  }

  mah_shutdown();
  return 0;
}

// Classifies the live position; returns false (and "ongoing") while the side to
// move has a legal move. A missing king counts as mate.
bool current_result(std::string& result) {
  Board& board = live_board();
  const BoardState& state = board.state;

  if(!state.bitboards[K] || !state.bitboards[k]) {
    result = !state.bitboards[K] ? "black_checkmate" : "white_checkmate";
    return true;
  }

  if(board.has_legal_move()) {
    result = "ongoing";
    return false;
  }

  if(board.in_check())
    result = state.side == white ? "black_checkmate" : "white_checkmate";
  else
    result = "stalemate";
  return true;
}

std::string board_fen() {
  return custom_position_fen(live_board());
}

int run_selfplay_command(const int games, const int depth, const int max_plies, const unsigned int seed, const char* json_path) {
  std::FILE* log_file = nullptr;
  bool first_logged_game = true;

  const auto abort_run = [&log_file](const char* message) {
    fail(message);
    if(log_file != nullptr) {
      std::fprintf(log_file, "\n]\n");
      std::fclose(log_file);
    }
    mah_shutdown();
    return 1;
  };

  std::srand(seed);

  if(!mah_init()) {
    fail("failed to initialize engine.");
    return 1;
  }

  if(json_path != nullptr) {
    log_file = std::fopen(json_path, "w");
    if(log_file == nullptr) {
      fail("failed to open JSON log file.");
      mah_shutdown();
      return 1;
    }
    std::fprintf(log_file, "[\n");
  }

  for(int game{ }; game < games; ++game) {
    const int side_to_move = std::rand() % 2;
    std::vector<std::string> move_history;
    std::array<char, fen_buffer_size> start_fen{ };
    std::array<char, move_buffer_size> move{ };
    std::string result;
    std::string end_fen;

    if(!mah_generate_custom_position_fen(side_to_move, seed + static_cast<unsigned int>(game), start_fen.data(), static_cast<int>(start_fen.size())))
      return abort_run("failed to generate a valid custom position.");

    if(!mah_set_position_fen(start_fen.data()))
      return abort_run("failed to load generated FEN.");

    std::printf("game %d start %s\n", game + 1, start_fen.data());

    for(int ply{ }; ply < max_plies; ++ply) {
      move.fill('\0');
      if(!mah_best_move_depth(depth, move.data(), static_cast<int>(move.size()))) {
        current_result(result);
        std::printf("game %d result %s at ply %d\n", game + 1, result.c_str(), ply);
        break;
      }

      std::printf("game %d ply %d %s\n", game + 1, ply + 1, move.data());
      move_history.emplace_back(move.data());
      if(!mah_apply_move(move.data()))
        return abort_run("engine produced an invalid move.");

      if(current_result(result)) {
        end_fen = board_fen();
        std::printf("game %d result %s fen %s\n", game + 1, result.c_str(), end_fen.c_str());
        break;
      }

      if(ply == max_plies - 1) {
        end_fen = board_fen();
        result = "move_limit";
        std::printf("game %d result move_limit fen %s\n", game + 1, end_fen.c_str());
      }
    }

    if(end_fen.empty()) {
      end_fen = board_fen();
      current_result(result);
    }

    if(log_file != nullptr) {
      json_write_game(log_file, first_logged_game, game + 1, start_fen.data(), result, end_fen, move_history);
      first_logged_game = false;
    }
  }

  if(log_file != nullptr) {
    std::fprintf(log_file, "\n]\n");
    std::fclose(log_file);
  }

  mah_shutdown();
  return 0;
}

struct BenchPosition {
  const char* name;
  std::string_view fen; // a literal, so data() is null-terminated
  int depth;
};

int run_bench_command() {
  static constexpr std::array<BenchPosition, 5> positions{ {
    { "startpos", Fen::start_position, 3 },
    { "archbishop_capture", "k7/5q2/8/4A3/8/8/8/K7 w - - 0 1", 2 },
    { "chancellor_capture", "7k/8/8/3C4/8/8/3q4/K7 w - - 0 1", 2 },
    { "amazon_capture", "k7/8/8/3M4/8/8/8/K6q w - - 0 1", 2 },
    { "defensive_escape", "6k1/8/8/8/3M4/8/6q1/6RK w - - 0 1", 2 },
  } };

  unsigned long long total_nodes{ };
  int total_time_ms{ };
  std::array<char, move_buffer_size> move{ };

  if(!mah_init()) {
    fail("failed to initialize engine.");
    return 1;
  }

  if(!mah_set_difficulty_level(5)) {
    fail("failed to set benchmark difficulty.");
    mah_shutdown();
    return 1;
  }

  std::printf("bench suite positions=%zu difficulty=5 mode=classic\n", positions.size());

  for(const BenchPosition& position : positions) {
    const int start_ms = now_ms();

    if(!mah_set_position_fen(position.fen.data())) {
      std::fprintf(stderr, "maharajah_tool: bench could not set FEN for %s.\n", position.name);
      mah_shutdown();
      return 1;
    }

    move.fill('\0');
    if(!mah_best_move_depth(position.depth, move.data(), static_cast<int>(move.size()))) {
      std::fprintf(stderr, "maharajah_tool: bench produced no move for %s.\n", position.name);
      mah_shutdown();
      return 1;
    }

    const int elapsed_ms = now_ms() - start_ms;
    const unsigned long long nodes = ffi::last_search_nodes();
    total_nodes += nodes;
    total_time_ms += elapsed_ms;

    std::printf("bench case=%s depth=%d move=%s nodes=%llu time_ms=%d", position.name, position.depth, move.data(), nodes, elapsed_ms);
    if(elapsed_ms > 0)
      std::printf(" nps=%llu", nodes * 1000ULL / static_cast<unsigned long long>(elapsed_ms));
    std::printf("\n");
  }

  std::printf("bench summary positions=%zu total_nodes=%llu total_time_ms=%d", positions.size(), total_nodes, total_time_ms);
  if(total_time_ms > 0)
    std::printf(" avg_nps=%llu", total_nodes * 1000ULL / static_cast<unsigned long long>(total_time_ms));
  std::printf("\n");

  mah_shutdown();
  return 0;
}

// ---------------------------------------------------------------------------
// UCI-subset REPL
//
// Drives the engine through the same mah_* functions the app calls, so
// Maharajah_lab can play it against Fairy-Stockfish with the measured subject
// identical to what the app plays, difficulty ladder included. Besides the UCI
// subset it answers three queries the match arbiter needs: `status`, `getfen` and
// `legalmoves`.
// ---------------------------------------------------------------------------

class UciSession {
  public:
  int run();

  private:
  bool set_position(std::string_view command);
  static bool apply_move_list(std::string_view moves);
  static void go(std::string_view command);
  static void status();
  static void getfen(std::string_view command);
  static void identify();
  static void setoption(std::string_view command);

  // The last `position` command executed, so an extension of it (the normal case
  // while a game is played out) is served by applying only the new moves; reloading
  // the FEN would clear the transposition table every ply.
  std::string last_position_;
};

std::string_view skip_spaces(std::string_view text) {
  const std::size_t start = text.find_first_not_of(" \t");
  return start == std::string_view::npos ? std::string_view{ } : text.substr(start);
}

// the integer after `key` (atoi semantics: 0 when there is none)
int number_after(const std::string_view text, const std::size_t key_end) {
  return std::atoi(std::string(text.substr(key_end)).c_str());
}

// Applies a whitespace-separated run of coordinate moves. Returns false at the
// first move the engine rejects, which leaves the board mid-sequence; the caller
// then treats the position as unusable.
bool UciSession::apply_move_list(std::string_view moves) {
  for(moves = skip_spaces(moves); !moves.empty(); moves = skip_spaces(moves)) {
    const std::size_t length = std::min(moves.find_first_of(" \t"), moves.size());
    const std::string move(moves.substr(0, length));
    if(move.size() >= 8) {
      fail("malformed move token.");
      return false;
    }
    if(!mah_apply_move(move.c_str())) {
      std::fprintf(stderr, "maharajah_tool: illegal move '%s'.\n", move.c_str());
      return false;
    }
    moves.remove_prefix(length);
  }

  return true;
}

// `position [startpos | fen <fen>] [moves ...]`, keeping the hash table whenever
// `command` merely extends the previous position command.
bool UciSession::set_position(const std::string_view command) {
  if(!last_position_.empty() && command.starts_with(last_position_)) {
    std::string_view extra = skip_spaces(command.substr(last_position_.size()));
    if(extra.starts_with("moves "))
      extra = skip_spaces(extra.substr(std::strlen("moves ")));
    if(apply_move_list(extra)) {
      last_position_ = command;
      return true;
    }
    // Fall through to a clean reload: the incremental path already changed the
    // board, so the cached command no longer describes it.
    last_position_.clear();
  }

  const std::string_view arguments = skip_spaces(command.substr(std::strlen("position")));
  const std::size_t moves = arguments.find("moves");

  if(arguments.starts_with("startpos")) {
    if(!mah_set_position_startpos())
      return false;
  } else if(arguments.starts_with("fen")) {
    // Our FEN has up to 8 fields and `moves` is the only terminator, so the field
    // count never has to be guessed here.
    const std::size_t fen_start = std::min(arguments.find_first_not_of(" \t", std::strlen("fen")), arguments.size());
    std::string_view fen = arguments.substr(fen_start, moves == std::string_view::npos ? std::string_view::npos : moves - fen_start);
    fen = fen.substr(0, fen.find_last_not_of(" \t") + 1);
    if(fen.empty() || fen.size() >= fen_buffer_size) {
      fail("FEN missing or too long.");
      return false;
    }
    const std::string fen_string(fen);
    if(!mah_set_position_fen(fen_string.c_str())) {
      std::fprintf(stderr, "maharajah_tool: failed to load FEN '%s'.\n", fen_string.c_str());
      return false;
    }
  } else {
    fail("position needs 'startpos' or 'fen'.");
    return false;
  }

  if(moves != std::string_view::npos && !apply_move_list(arguments.substr(moves + std::strlen("moves"))))
    return false;

  last_position_ = command;
  return true;
}

void UciSession::go(const std::string_view command) {
  std::array<char, move_buffer_size> move{ };
  const std::size_t movetime = command.find("movetime");
  const std::size_t depth = command.find("depth");
  int found;

  if(movetime != std::string_view::npos)
    found = mah_best_move_time(number_after(command, movetime + std::strlen("movetime")), move.data(), static_cast<int>(move.size()));
  else if(depth != std::string_view::npos)
    found = mah_best_move_depth(number_after(command, depth + std::strlen("depth")), move.data(), static_cast<int>(move.size()));
  else
    // no search limit: a fixed, modest default keeps a stray `go` from running forever
    found = mah_best_move_time(1000, move.data(), static_cast<int>(move.size()));

  std::printf("bestmove %s\n", found ? move.data() : "(none)");
}

void UciSession::status() {
  switch(mah_game_status()) {
    case mah_status_checkmate:
      std::printf("status checkmate\n");
      break;
    case mah_status_stalemate:
      std::printf("status stalemate\n");
      break;
    default:
      std::printf("status ongoing\n");
      break;
  }
}

void UciSession::getfen(const std::string_view command) {
  const std::string_view argument = skip_spaces(command.substr(std::strlen("getfen")));
  const int fullmove_number = argument.empty() ? 1 : number_after(argument, 0);
  std::array<char, fen_buffer_size> fen{ };

  if(mah_get_fen(fen.data(), static_cast<int>(fen.size()), fullmove_number > 0 ? fullmove_number : 1))
    std::printf("fen %s\n", fen.data());
  else
    std::printf("fen (none)\n");
}

void UciSession::identify() {
  std::printf("id name Maharajah %s (tool)\n", MAHARAJAH_VERSION);
  std::printf("id author Villi\n");
  std::printf("option name Hash type spin default 64 min 4 max 1024\n");
  std::printf("option name Skill Level type spin default 10 min 1 max 10\n");
  std::printf("option name UI Difficulty type spin default 5 min 1 max 5\n");
  std::printf("uciok\n");
}

void UciSession::setoption(const std::string_view command) {
  const std::size_t value = command.find(" value ");
  if(value == std::string_view::npos) {
    fail("setoption without a value.");
    return;
  }

  const int number = number_after(command, value + std::strlen(" value "));
  if(command.find("name Hash") != std::string_view::npos)
    mah_set_hash_mb(number);
  else if(command.find("name Skill Level") != std::string_view::npos)
    mah_set_skill_level(number);
  else if(command.find("name UI Difficulty") != std::string_view::npos)
    mah_set_difficulty_level(number);
  else
    std::fprintf(stderr, "maharajah_tool: unknown option in '%s'.\n", std::string(command).c_str());
}

int UciSession::run() {
  std::setbuf(stdout, nullptr);

  if(!mah_init()) {
    fail("failed to initialize engine.");
    return 1;
  }
  last_position_.clear();

  std::string input;
  while(std::getline(std::cin, input)) {
    std::string_view line = input;
    line = line.substr(0, line.find_last_not_of(" \t\r\n") + 1);
    if(line.empty())
      continue;

    if(line == "quit") {
      break;
    } else if(line == "uci") {
      identify();
    } else if(line == "isready") {
      std::printf("readyok\n");
    } else if(line == "ucinewgame") {
      mah_set_position_startpos();
      last_position_.clear();
    } else if(line.starts_with("setoption")) {
      setoption(line);
    } else if(line.starts_with("position")) {
      if(!set_position(line)) {
        last_position_.clear();
        std::printf("error position\n");
      }
    } else if(line.starts_with("go")) {
      go(line);
    } else if(line == "status") {
      status();
    } else if(line.starts_with("getfen")) {
      getfen(line);
    } else if(line == "legalmoves") {
      print_legal_moves();
      std::printf("endmoves\n");
    } else {
      std::fprintf(stderr, "maharajah_tool: unknown command '%s'.\n", std::string(line).c_str());
    }
  }

  mah_shutdown();
  return 0;
}

void print_usage(const char* program) {
  std::fprintf(stderr,
               "Usage:\n"
               "  %s generate [count] [seed]\n"
               "  %s selfplay [games] [depth] [max_plies] [seed] [json_path]\n"
               "  %s legalmoves <fen>\n"
               "  %s bench\n"
               "  %s uci\n",
               program,
               program,
               program,
               program,
               program);
}

} // namespace

int main(int argc, char* argv[]) {
  if(argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const std::string_view command = argv[1];

  if(command == "generate") {
    const int count = argc >= 3 ? std::atoi(argv[2]) : 1;
    const unsigned int seed = argc >= 4 ? static_cast<unsigned int>(std::strtoul(argv[3], nullptr, 10)) : 1U;
    return run_generate_command(count > 0 ? count : 1, seed);
  }

  if(command == "selfplay") {
    const int games = argc >= 3 ? std::atoi(argv[2]) : 1;
    const int depth = argc >= 4 ? std::atoi(argv[3]) : 1;
    const int max_plies = argc >= 5 ? std::atoi(argv[4]) : 100;
    const unsigned int seed = argc >= 6 ? static_cast<unsigned int>(std::strtoul(argv[5], nullptr, 10)) : 1U;
    const char* json_path = argc >= 7 ? argv[6] : nullptr;
    return run_selfplay_command(games > 0 ? games : 1, depth > 0 ? depth : 1, max_plies > 0 ? max_plies : 100, seed, json_path);
  }

  if(command == "legalmoves") {
    if(argc < 3) {
      print_usage(argv[0]);
      return 1;
    }
    return run_legalmoves_command(argv[2]);
  }

  if(command == "bench")
    return run_bench_command();

  if(command == "uci")
    return UciSession().run();

  print_usage(argv[0]);
  return 1;
}
