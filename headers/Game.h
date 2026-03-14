#pragma once

#include "../headers/Board.h"

#include <string>
#include <string_view>

namespace maharajah {

enum GameState { start_game, play_game, end_game };

struct Game {
  Game();
  void play();
  int shutdown();
  [[nodiscard]] GameState state() const;
  [[nodiscard]] std::string print_board(bool print_to_console = true) const;
  void print_attacked_squares(Colors side) const;
  void parse_fen(std::string_view fen);
  static std::string print_bitboard(u64 bitboard, bool print_to_console = false);
  static void print_move(int move);
  void print_move_list();
  int get_time_ms();

#ifdef MAHARAJAH_TESTING
  friend struct GameTestAccess;
#endif

  private:
  Board board_{};
  GameState game_state_{ start_game };
  int score_{ 0 };
};

} // namespace maharajah
