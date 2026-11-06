#include "../headers/Endgame.h"
#include "../headers/Bitboard.h"
#include "../headers/Evaluation.h"

#include <algorithm>
#include <cstdlib>

namespace maharajah::Endgame {

namespace {

// a8 is square 0 and light, a1 (square 56) dark
constexpr u64 light_squares{ 0xAA55AA55AA55AA55ULL };
constexpr u64 file_a{ 0x0101010101010101ULL };
constexpr u64 file_h{ file_a << 7 };

int file_of(const int square) { return square & 7; }
// 0 = rank 1 .. 7 = rank 8 (a8 is square 0)
int rank_of(const int square) { return 7 - (square >> 3); }
bool is_light(const int square) { return (light_squares >> square) & 1; }

int chebyshev(const int a, const int b) {
  return std::max(std::abs(file_of(a) - file_of(b)), std::abs(rank_of(a) - rank_of(b)));
}

int manhattan(const int a, const int b) {
  return std::abs(file_of(a) - file_of(b)) + std::abs(rank_of(a) - rank_of(b));
}

// 0 in the four centre squares .. 6 in a corner
int centre_distance(const int square) {
  const int file = file_of(square), rank = rank_of(square);
  return std::max(3 - file, file - 4) + std::max(3 - rank, rank - 4);
}

int count(const BoardState& state, const Pieces white_piece, const Colors side) {
  return count_bits(state.bitboards[colored(white_piece, side)]);
}

// non-pawn, non-king material
int piece_material(const BoardState& state, const Colors side) {
  int material{ };
  for(const Pieces piece : { N, B, R, Q, A, C, M })
    material += count(state, piece, side) * Evaluation::piece_value[piece];
  return material;
}

bool has_bishops_on_both_colours(const BoardState& state, const Colors side) {
  const u64 bishops = state.bitboards[colored(B, side)];
  return (bishops & light_squares) && (bishops & ~light_squares);
}

// mate can be forced against a bare king without promoting a pawn
bool has_mating_pieces(const BoardState& state, const Colors side) {
  for(const Pieces piece : { R, Q, A, C, M }) {
    if(count(state, piece, side))
      return true;
  }
  const int knights = count(state, N, side);
  return (count(state, B, side) && knights) || has_bishops_on_both_colours(state, side) || knights >= 3;
}

// white's score for `strong` winning against a bare king
int bare_king_score(const BoardState& state, const Colors strong) {
  const Colors weak = opponent(strong);
  const int strong_king = get_ls1b_index(state.bitboards[colored(K, strong)]);
  const int weak_king = get_ls1b_index(state.bitboards[colored(K, weak)]);

  int score = known_win + piece_material(state, strong);

  // pawns are worth more the closer they are to promoting
  u64 pawns = state.bitboards[colored(P, strong)];
  while(pawns) {
    const int square = get_ls1b_index(pawns);
    pawns &= pawns - 1;
    const int advance = strong == white ? rank_of(square) : 7 - rank_of(square);
    score += Evaluation::piece_value[P] + 20 * advance;
  }

  // bishop + knight mates only in a corner of the bishop's colour
  const bool bishop_knight_only = count(state, B, strong) == 1 && count(state, N, strong) == 1 && !state.bitboards[colored(P, strong)] &&
                                  piece_material(state, strong) == Evaluation::piece_value[B] + Evaluation::piece_value[N];
  if(bishop_knight_only) {
    const bool light = is_light(get_ls1b_index(state.bitboards[colored(B, strong)]));
    // a1 and h8 are dark, a8 and h1 light
    const int corner_a = light ? a8 : a1;
    const int corner_b = light ? h1 : h8;
    score += 200 * (14 - std::min(manhattan(weak_king, corner_a), manhattan(weak_king, corner_b)));
  } else {
    score += 20 * centre_distance(weak_king);
  }
  score += 10 * (14 - manhattan(strong_king, weak_king));

  return strong == white ? score : -score;
}

// how much of a score the stronger side keeps, out of 64
int draw_scale(const BoardState& state, const Colors strong) {
  const Colors weak = opponent(strong);
  const u64 strong_pawns = state.bitboards[colored(P, strong)];
  const int strong_material = piece_material(state, strong);
  const int weak_material = piece_material(state, weak);

  if(!strong_pawns) {
    // two knights cannot force mate against a bare king
    if(strong_material == 2 * Evaluation::piece_value[N] && count(state, N, strong) == 2 && !weak_material && !state.bitboards[colored(P, weak)])
      return 2;
    // KR v KB, KR v KN, KB v KN, ... (and anything down to K v K)
    if(strong_material - weak_material <= Evaluation::piece_value[B])
      return 8;
    return 64;
  }

  // the wrong bishop: bishop and rook pawns of one file, promotion square of the
  // other colour, held by the defending king
  if(strong_material == Evaluation::piece_value[B] && count(state, B, strong) == 1) {
    const bool a_file = !(strong_pawns & ~file_a);
    const bool h_file = !(strong_pawns & ~file_h);
    if(a_file || h_file) {
      const int promotion = strong == white ? (a_file ? a8 : h8) : (a_file ? a1 : h1);
      const bool bishop_light = is_light(get_ls1b_index(state.bitboards[colored(B, strong)]));
      const int weak_king = get_ls1b_index(state.bitboards[colored(K, weak)]);
      if(bishop_light != is_light(promotion) && chebyshev(weak_king, promotion) <= 1)
        return 2;
    }
  }
  return 64;
}

} // namespace

bool is_bare_king_win(const BoardState& state) {
  const u64 white_king = state.bitboards[K];
  const u64 black_king = state.bitboards[k];
  if(!white_king || !black_king)
    return false;
  const bool white_bare = state.occupancies[white] == white_king;
  const bool black_bare = state.occupancies[black] == black_king;
  return white_bare != black_bare && has_mating_pieces(state, white_bare ? black : white);
}

int adjust(const BoardState& state, const int score) {
  const u64 white_king = state.bitboards[K];
  const u64 black_king = state.bitboards[k];
  // a captured king is scored by the search
  if(!white_king || !black_king)
    return score;

  const bool white_bare = state.occupancies[white] == white_king;
  const bool black_bare = state.occupancies[black] == black_king;

  if(white_bare != black_bare) {
    const Colors strong = white_bare ? black : white;
    if(has_mating_pieces(state, strong)) {
      const int white_score = bare_king_score(state, strong);
      return state.side == white ? white_score : -white_score;
    }
  }

  // only the side the score favours is pulled towards a draw
  if(score == 0)
    return score;
  const Colors strong = (score > 0) == (state.side == white) ? white : black;

  // Fast exit, taken in nearly every middlegame position (this runs on every
  // evaluation): with pawns, the only rule left is the wrong bishop, which needs
  // exactly one bishop and no other piece besides the pawns.
  if(const u64 strong_pawns = state.bitboards[colored(P, strong)]) {
    const u64 pieces = state.occupancies[strong] ^ strong_pawns ^ state.bitboards[colored(K, strong)];
    if(pieces != state.bitboards[colored(B, strong)] || count_bits(pieces) != 1)
      return score;
  }

  // the draw rules below are orthodox chess knowledge
  for(const Pieces piece : { A, C, M }) {
    if(state.bitboards[piece] || state.bitboards[colored(piece, black)])
      return score;
  }
  const int scale = draw_scale(state, strong);
  return scale == 64 ? score : score * scale / 64;
}

} // namespace maharajah::Endgame
