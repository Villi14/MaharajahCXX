#pragma once

#include "BoardState.h"
#include "MoveList.h"

#include <cassert>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace maharajah {

struct Board {
  int ply{ };
  BoardState state{ };
  // states saved by make_move, restored by pop_state; history[ply - 1] is the
  // position before the last move
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
  // Draw by repetition, the fifty/seventy-five move rules or insufficient material.
  [[nodiscard]] bool is_draw() const;
  [[nodiscard]] bool is_insufficient_material() const;

  [[nodiscard]] bool is_square_attacked(Squares square, Colors side) const;
  [[nodiscard]] bool in_check() const;
  // Saves the state and plays `move`; false (with the state restored) if it is illegal
  // or, with only_captures, not a capture. pop_state takes a made move back.
  bool make_move(int move, TypeMove type_move);
  // Saves the state and passes the turn (for null-move pruning); pop_state undoes it.
  void make_null_move();
  void update_occupancies();
  // pseudo-legal moves; with only_captures, just the captures (promotions included)
  void generate_moves(MoveList& out_moves, TypeMove type_move = TypeMove::all_moves) const;
  // true when the side to move has at least one legal move
  [[nodiscard]] bool has_legal_move();

  // Loads a FEN with the optional 7th (per-side variant rights: 'V' white, 'v' black,
  // '-' none) and 8th (unmoved pawn squares, e.g. "e2d7" or '-') fields.
  // A malformed FEN leaves the board untouched; the error says what is wrong with it
  // ("invalid fen: castling rights").
  std::expected<void, std::string_view> parse_fen(std::string_view fen);
  // Serializes the position; field 7 is always written, field 8 only when the
  // position carries per-pawn state.
  [[nodiscard]] std::string to_fen(int fullmove_number = 1) const;
  // Parses a move in coordinate notation ("e2e4", "e7e8q"); returns 0 if it matches
  // no pseudo-legal move. Anything after the first whitespace is ignored.
  [[nodiscard]] int parse_move(std::string_view move_string) const;
  [[nodiscard]] static std::string move_to_string(int move);
};

} // namespace maharajah
