#pragma once

#include "BoardState.h"
#include "MoveList.h"

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace maharajah {

struct Board {
  int ply{ };
  BoardState state{ };
  MoveList moves_list{ };
  std::vector<BoardState> history{ };

  // Hash keys of earlier positions for repetition detection: entry i holds the key
  // of the position before the i-th move of the game (or of the search line).
  // Entry 0 is never written.
  std::vector<u64> repetition_table{ 0 };
  int repetition_index{ };

  void push_state() {
    if(ply < static_cast<int>(history.size()))
      history[ply] = state;
    else
      history.push_back(state);
    ++ply;
  }

  void pop_state() {
    assert(ply > 0);
    state = history[--ply];
  }

  // records the current position before a move is made on top of it
  void remember_position() {
    ++repetition_index;
    if(repetition_index >= static_cast<int>(repetition_table.size()))
      repetition_table.resize(repetition_index + 1);
    repetition_table[repetition_index] = state.hash_key;
  }

  void forget_position() {
    --repetition_index;
  }

  // Occurrences of the current position, counting itself.
  [[nodiscard]] int repetition_count() const;

  [[nodiscard]] bool is_square_attacked(Squares square, Colors side) const;
  [[nodiscard]] bool in_check() const;
  bool make_move(int move, TypeMove type_move);
  void update_occupancies();
  void generate_moves();
  void generate_moves(MoveList& moves_list) const;
  // true when the side to move has at least one legal move
  [[nodiscard]] bool has_legal_move();

  // Loads a FEN with the optional 7th (per-side variant rights: 'V' white, 'v' black,
  // '-' none) and 8th (unmoved pawn squares, e.g. "e2d7" or '-') fields.
  // Throws std::runtime_error on a malformed FEN and leaves the board untouched.
  void parse_fen(std::string_view fen);
  // Serializes the position; field 7 is always written, field 8 only when the
  // position carries per-pawn state.
  [[nodiscard]] std::string to_fen(int fullmove_number = 1) const;
  // Parses a move in coordinate notation ("e2e4", "e7e8q"); returns 0 if it matches
  // no pseudo-legal move. Anything after the first whitespace is ignored.
  [[nodiscard]] int parse_move(std::string_view move_string) const;
  [[nodiscard]] static std::string move_to_string(int move);
};

} // namespace maharajah
