#pragma once

#include "Constants.h"

/*         binary move bits                             hexidecimal constants

    0000 0000 0000 0000 0011 1111    source square       0x3f
    0000 0000 0000 1111 1100 0000    target square       0xfc0
    0000 0000 1111 0000 0000 0000    piece               0xf000
    0000 1111 0000 0000 0000 0000    promoted piece      0xf0000
    0001 0000 0000 0000 0000 0000    capture flag        0x100000
    0010 0000 0000 0000 0000 0000    double push flag    0x200000
    0100 0000 0000 0000 0000 0000    enpassant flag      0x400000
    1000 0000 0000 0000 0000 0000    castling flag       0x800000
*/
namespace maharajah {

struct Move {
  Squares source;
  Squares target;
  Pieces piece;
  Pieces promoted;
  bool capture;
  bool move_double;
  bool enpassant;
  bool castling;

  // clang-format off
  static int encode_move(const Move& move) {
    return move.source |
           (move.target << 6) |
           (move.piece << 0xC) |
           (move.promoted << 0x10) |
           (move.capture << 0x14) |
           (move.move_double << 0x15) |
           (move.enpassant << 0x16) |
           (move.castling << 0x17);
  }

  static Move decode_move(const int move) {
    return Move{ to_square(move & 0x3f),
                 to_square((move & 0xfc0) >> 6),
                 to_piece((move & 0xf000) >> 0xC),
                 to_piece((move & 0xf0000) >> 0x10),
                 (move & 0x100000) ? true : false,
                 (move & 0x200000) ? true : false,
                 (move & 0x400000) ? true : false,
                 (move & 0x800000) ? true : false };
  }
  // clang-format on

  static Squares get_move_source(const int move) {
    return to_square(move & 0x3f);
  }

  static Squares get_move_target(const int move) {
    return to_square((move & 0xfc0) >> 6);
  }

  static Pieces get_move_piece(const int move) {
    return to_piece((move & 0xf000) >> 0xC);
  }

  static Pieces get_move_promoted(const int move) {
    return to_piece((move & 0xf0000) >> 0x10);
  }

  static bool get_move_capture(const int move) {
    return (move & 0x100000) ? true : false;
  }

  static bool get_move_double(const int move) {
    return (move & 0x200000) ? true : false;
  }

  static bool get_move_enpassant(const int move) {
    return (move & 0x400000) ? true : false;
  }

  static bool get_move_castling(const int move) {
    return (move & 0x800000) ? true : false;
  }
};

} // namespace maharajah
