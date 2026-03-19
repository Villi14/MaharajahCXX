#pragma once

#include "../headers/Board.h"

#include <iostream>
#include <cstring>
#include <string>
#include <string_view>

namespace maharajah {

enum GameState { start_game, play_game, end_game };

struct Game {
  Game();
  void play();
  int shutdown();
  [[nodiscard]] GameState state() const;
  std::string print_board(bool print_to_console = true) const;
  void print_attacked_squares(Colors side) const;
  void parse_fen(std::string_view fen);
  static std::string print_bitboard(u64 bitboard, bool print_to_console = false);
  static void print_move(int move);
  void print_move_list();
  static int get_time_ms();
  void perft_driver(int depth);
  [[nodiscard]] u64 perft(int depth);
  void perft_divide(int depth);
  void perft_test(int depth);
  int parse_move(const char* move_string);
  void parse_go(const char* command);
  void search_position(int depth);
  void parse_position(char* command);
  void uci_loop();
  int negamax(int alpha, int beta, int depth);

  static void print_uci_info() {
    std::cout << "id name Maharajah\n";
    std::cout << "id author Villi\n";
    std::cout << "uciok\n";
  }

  static bool starts_with(const char* str, const char* cmd) {
    while(*cmd) {
      if(*str++ != *cmd++)
        return false;
    }
    return true;
  }

  static bool is_token(const char* str, const char* token) {
    size_t len = std::strlen(token);
    return std::strncmp(str, token, len) == 0 && (str[len] == ' ' || str[len] == '\n' || str[len] == '\0');
  }

  int evaluate();

#ifdef MAHARAJAH_TESTING
  friend struct GameTestAccess;
#endif

  private:
  Board board_{};
  GameState game_state_{ start_game };
  int score_{};
  long nodes_{};
};

} // namespace maharajah
