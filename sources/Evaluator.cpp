#include "../headers/Evaluator.h"
#include "../headers/Bitboard.h"

namespace maharajah {

// position evaluation
int Evaluator::evaluate(const BoardState& state) {
  // static evaluation score
  int score = 0;

  // current pieces bitboard copy
  u64 bitboard;

  Pieces piece;
  Squares square;

  // loop over piece bitboards
  for(Pieces bb_piece{ P }; bb_piece <= k; ++bb_piece) {
    // init piece bitboard copy
    bitboard = state.bitboards[bb_piece];

    // loop over pieces within a bitboard
    while(bitboard) {
      piece = bb_piece;
      square = get_ls1b_index(bitboard);
      score += Evaluation::material_score[piece];

      // score positional piece scores
      switch(piece) {
      // evaluate white pieces
      case P:
        score += Evaluation::pawn_score[square];
        break;
      case N:
        score += Evaluation::knight_score[square];
        break;
      case B:
        score += Evaluation::bishop_score[square];
        break;
      case R:
        score += Evaluation::rook_score[square];
        break;
      case K:
        score += Evaluation::king_score[square];
        break;

      // evaluate black pieces
      case p:
        score -= Evaluation::pawn_score[Evaluation::mirror_score[square]];
        break;
      case n:
        score -= Evaluation::knight_score[Evaluation::mirror_score[square]];
        break;
      case b:
        score -= Evaluation::bishop_score[Evaluation::mirror_score[square]];
        break;
      case r:
        score -= Evaluation::rook_score[Evaluation::mirror_score[square]];
        break;
      case k:
        score -= Evaluation::king_score[Evaluation::mirror_score[square]];
        break;

      // queens and empty squares have no positional table
      default:
        break;
      }

      pop_bit(bitboard, square);
    }
  }

  // return final evaluation based on side
  return (state.side == white) ? score : -score;
}

} // namespace maharajah
