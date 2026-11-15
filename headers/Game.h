#pragma once

#include "Engine.h"

#include <deque>
#include <string>
#include <string_view>

namespace maharajah {

enum GameState { start_game, play_game, end_game };

struct Game {
  static constexpr int default_hash_mb{ 64 };
  static constexpr int min_hash_mb{ 4 };
  static constexpr int max_hash_mb{ 1024 };

  Game();
  // the search's input callback refers to this object
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
  void play(bool debug = false);
  int shutdown();
  [[nodiscard]] GameState state() const;
  std::string print_board(bool print_to_console = true) const;
  void print_attacked_squares(Colors side) const;
  std::expected<void, std::string_view> parse_fen(std::string_view fen);
  static std::string print_bitboard(u64 bitboard, bool print_to_console = false);
  static void print_move(int move);
  // the pseudo-legal moves of the current position
  void print_move_list() const;
  int parse_move(std::string_view move_string) const;
  void parse_go(std::string_view command);
  // Searches and prints "bestmove". With `wait_for_stop` ("go infinite") a search that
  // ends by itself still holds the move back until "stop" or "quit" arrives.
  void search_position(int depth, bool wait_for_stop = false);
  void parse_position(std::string_view command);
  void uci_loop();

  static void print_uci_info();

#ifdef MAHARAJAH_TESTING
  friend struct GameTestAccess;
#endif

  private:
  // Reads one line that arrived during a search: "stop" and "quit" (or the end of the
  // input) end the search, "isready" is answered at once, anything else waits in
  // pending_ until the search is over.
  void read_search_input();
  // the next command: one that arrived during a search, else a line of input
  bool next_command(std::string& command);

  Engine engine_{ };
  // commands that arrived during a search, oldest first
  std::deque<std::string> pending_{ };
  // "stop" or "quit" arrived for the current search
  bool stop_requested_{ };
  GameState game_state_{ start_game };
  int score_{ };
  bool verbose_{ false };
};

} // namespace maharajah
