#pragma once

#include "Engine.h"

#include <string>
#include <string_view>

namespace maharajah {

enum GameState { start_game, play_game, end_game };

struct Game {
  static constexpr int default_hash_mb{ 64 };
  static constexpr int min_hash_mb{ 4 };
  static constexpr int max_hash_mb{ 128 };

  Game();
  void play(bool debug = false);
  int shutdown();
  [[nodiscard]] GameState state() const;
  std::string print_board(bool print_to_console = true) const;
  void print_attacked_squares(Colors side) const;
  void parse_fen(std::string_view fen);
  static std::string print_bitboard(u64 bitboard, bool print_to_console = false);
  static void print_move(int move);
  void print_move_list();
  int parse_move(std::string_view move_string) const;
  void parse_go(std::string_view command);
  void search_position(int depth);
  void parse_position(std::string_view command);
  void uci_loop();

  static void print_uci_info();

  static bool starts_with(std::string_view str, std::string_view cmd) {
    return str.starts_with(cmd);
  }

  static bool is_token(std::string_view str, std::string_view token) {
    return str.starts_with(token) && (str.size() == token.size() || str[token.size()] == ' ' || str[token.size()] == '\n');
  }

#ifdef MAHARAJAH_TESTING
  friend struct GameTestAccess;
#endif

  private:
  Engine engine_{ };
  GameState game_state_{ start_game };
  int score_{ };
  bool verbose_{ false };
};

} // namespace maharajah
