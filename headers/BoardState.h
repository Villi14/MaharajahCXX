#pragma once

#include "Types.h"

#include <array>

namespace maharajah {

// The piece among `first`..`last` standing on `square`, or no_pieces.
[[nodiscard]] inline Pieces find_piece(const std::array<u64, PieceCount::all>& bitboards, const Squares square, const Pieces first = P, const Pieces last = k) {
  const u64 mask = one << square;
  for(Pieces piece{ first }; piece <= last; ++piece) {
    if(bitboards[piece] & mask)
      return piece;
  }
  return no_pieces;
}

struct BoardState {
  Colors side{ white };
  int en_passant{ no_square };
  int castle{ };
  std::array<u64, PieceCount::all> bitboards{ };
  std::array<u64, 3> occupancies{ };
  int halfmove{ };
  // board-wide rules for the draw clocks (fifty-move rule, insufficient material);
  // off as soon as compound pieces are on the board
  bool standard_rules{ true };
  // Per-side rules in a mixed game, indexed by colour: variant rules allow compound
  // promotions (A/C/M) and pawn double steps from any rank, and forbid castling.
  // Carried by the optional 7th FEN field.
  std::array<bool, 2> side_variant{ };
  // Squares of the variant sides' pawns that have never moved (FEN field 8): the only
  // source of a variant pawn's double step, since a custom army's pawns may start off
  // rank 2/7. A classic side's pawns are never listed; their rank is their history.
  u64 pawn_unmoved{ };
  u64 hash_key{ };

  // Same position for repetition purposes (ignores the halfmove clock).
  [[nodiscard]] bool same_position(const BoardState& other) const {
    return side == other.side && castle == other.castle && en_passant == other.en_passant && bitboards == other.bitboards;
  }

  // the pawns that field 8 may list
  [[nodiscard]] u64 variant_pawns() const {
    return (side_variant[white] ? bitboards[P] : zero) | (side_variant[black] ? bitboards[p] : zero);
  }

  [[nodiscard]] bool has_compound_pieces() const {
    return has_compound_pieces(white) || has_compound_pieces(black);
  }

  [[nodiscard]] bool has_compound_pieces(const Colors color) const {
    return bitboards[colored(A, color)] || bitboards[colored(C, color)] || bitboards[colored(M, color)];
  }

  // the piece on `square`, or no_pieces
  [[nodiscard]] Pieces piece_on(const Squares square) const {
    return find_piece(bitboards, square);
  }

  // the piece of `color` on `square`, or no_pieces
  [[nodiscard]] Pieces piece_on(const Squares square, const Colors color) const {
    return find_piece(bitboards, square, colored(P, color), colored(K, color));
  }

  // the side's pieces other than its pawns and king
  [[nodiscard]] u64 non_pawn_pieces(const Colors color) const {
    return occupancies[color] & ~(bitboards[colored(P, color)] | bitboards[colored(K, color)]);
  }
};

} // namespace maharajah
