#pragma once

#include "Types.h"

#include <array>

namespace maharajah {

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
  // Squares holding a pawn that has never moved. Only consulted for pawn double
  // steps when has_pawn_state is set (optional 8th FEN field); a custom army's pawns
  // may start off rank 2/7, so the rank alone can't tell whether a pawn has moved.
  u64 pawn_unmoved{ };
  bool has_pawn_state{ };
  u64 hash_key{ };

  // Same position for repetition purposes (ignores the halfmove clock).
  [[nodiscard]] bool same_position(const BoardState& other) const {
    return side == other.side && castle == other.castle && en_passant == other.en_passant && bitboards == other.bitboards;
  }

  // A variant side's pawns may double-step from any rank, but only once, so a
  // position with a variant side always tracks its unmoved pawns. Without that state
  // (a FEN without field 8), a variant side's pawns on its own half count as unmoved
  // (an army starts there) and a standard side's pawns on its home rank.
  void infer_pawn_state() {
    if(has_pawn_state || (!side_variant[white] && !side_variant[black]))
      return;

    constexpr u64 white_half{ 0xFFFFFFFF00000000ULL }, black_half{ 0x00000000FFFFFFFFULL };
    constexpr u64 white_home_rank{ 0x00FF000000000000ULL }, black_home_rank{ 0x000000000000FF00ULL };
    pawn_unmoved = (bitboards[P] & (side_variant[white] ? white_half : white_home_rank)) | (bitboards[p] & (side_variant[black] ? black_half : black_home_rank));
    has_pawn_state = true;
  }

  [[nodiscard]] bool has_compound_pieces() const {
    return bitboards[A] || bitboards[C] || bitboards[M] || bitboards[a] || bitboards[c] || bitboards[m];
  }
};

} // namespace maharajah
