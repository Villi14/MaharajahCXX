#pragma once

#include "Types.h"

/*         binary move bits                             hexidecimal constants

    0000 0000 0000 0000 0000 0011 1111    source square       0x3f
    0000 0000 0000 0000 1111 1100 0000    target square       0xfc0
    0000 0000 0001 1111 0000 0000 0000    piece               0x1f000
    0000 0011 1110 0000 0000 0000 0000    promoted piece      0x3e0000
    0000 0100 0000 0000 0000 0000 0000    capture flag        0x400000
    0000 1000 0000 0000 0000 0000 0000    double push flag    0x800000
    0001 0000 0000 0000 0000 0000 0000    enpassant flag      0x1000000
    0010 0000 0000 0000 0000 0000 0000    castling flag       0x2000000

    A move without promotion stores no_pieces as the promoted piece, so no real
    move ever encodes to 0 and 0 can mean "no move".
*/
namespace maharajah {

struct Move {
  Squares source{ };
  Squares target{ };
  Pieces piece{ };
  // no_pieces for a move without promotion
  Pieces promoted{ no_pieces };
  bool capture{ };
  bool move_double{ };
  bool enpassant{ };
  bool castling{ };

  // bit layout of an encoded move (see the table above)
  static constexpr int square_mask{ 0x3f };
  static constexpr int piece_mask{ 0x1f };
  static constexpr int target_shift{ 6 };
  static constexpr int piece_shift{ 12 };
  static constexpr int promoted_shift{ 17 };
  static constexpr int capture_flag{ 1 << 22 };
  static constexpr int double_push_flag{ 1 << 23 };
  static constexpr int en_passant_flag{ 1 << 24 };
  static constexpr int castling_flag{ 1 << 25 };

  // clang-format off
  static int encode_move(const Move& move) {
    return move.source |
           (move.target << target_shift) |
           (move.piece << piece_shift) |
           (move.promoted << promoted_shift) |
           (move.capture ? capture_flag : 0) |
           (move.move_double ? double_push_flag : 0) |
           (move.enpassant ? en_passant_flag : 0) |
           (move.castling ? castling_flag : 0);
  }

  static Move decode_move(const int move) {
    return Move{ get_move_source(move),
                 get_move_target(move),
                 get_move_piece(move),
                 get_move_promoted(move),
                 get_move_capture(move),
                 get_move_double(move),
                 get_move_enpassant(move),
                 get_move_castling(move) };
  }
  // clang-format on

  // Encoded moves of each kind, as the move generator produces them.
  static int quiet_move(const Squares source, const Squares target, const Pieces piece) {
    return encode_move({ .source = source, .target = target, .piece = piece });
  }

  static int capture_move(const Squares source, const Squares target, const Pieces piece) {
    return encode_move({ .source = source, .target = target, .piece = piece, .capture = true });
  }

  static int double_push_move(const Squares source, const Squares target, const Pieces pawn) {
    return encode_move({ .source = source, .target = target, .piece = pawn, .move_double = true });
  }

  // `target` is the square the pawn moves to, not the square of the pawn it takes
  static int en_passant_move(const Squares source, const Squares target, const Pieces pawn) {
    return encode_move({ .source = source, .target = target, .piece = pawn, .capture = true, .enpassant = true });
  }

  // the king's move; make_move moves the rook
  static int castling_move(const Squares source, const Squares target, const Pieces king) {
    return encode_move({ .source = source, .target = target, .piece = king, .castling = true });
  }

  static int promotion_move(const Squares source, const Squares target, const Pieces pawn, const Pieces promoted, const bool capture) {
    return encode_move({ .source = source, .target = target, .piece = pawn, .promoted = promoted, .capture = capture });
  }

  static Squares get_move_source(const int move) {
    return to_square(move & square_mask);
  }

  static Squares get_move_target(const int move) {
    return to_square((move >> target_shift) & square_mask);
  }

  static Pieces get_move_piece(const int move) {
    return to_piece((move >> piece_shift) & piece_mask);
  }

  static Pieces get_move_promoted(const int move) {
    return to_piece((move >> promoted_shift) & piece_mask);
  }

  static bool get_move_capture(const int move) {
    return (move & capture_flag) != 0;
  }

  static bool get_move_double(const int move) {
    return (move & double_push_flag) != 0;
  }

  static bool get_move_enpassant(const int move) {
    return (move & en_passant_flag) != 0;
  }

  static bool get_move_castling(const int move) {
    return (move & castling_flag) != 0;
  }

  static bool is_promotion(const int move) {
    return get_move_promoted(move) != no_pieces;
  }
};

} // namespace maharajah
