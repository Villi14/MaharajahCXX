#include "../headers/See.h"
#include "../headers/Bitboard.h"
#include "../headers/Evaluation.h"
#include "../headers/Move.h"

#include <cstdlib>

namespace maharajah {

namespace {

using Bitboards = std::array<u64, PieceCount::all>;

int material_cp(const int piece) {
  return std::abs(Evaluation::material_score[opening][piece]);
}

bool piece_reaches(const Pieces piece, const Squares from, const Squares to, const u64 occupancy) {
  const u64 target = one << to;

  if(piece == P)
    return attack_tables.pawn[white][from] & target;
  if(piece == p)
    return attack_tables.pawn[black][from] & target;
  return get_piece_attacks(piece, from, occupancy) & target;
}

// least valuable attacker of `target` for side `us`: (piece << 8) | square, or -1
int find_lva(const Colors us, const Bitboards& bb, const u64 occupancy, const Squares target) {
  int best_from = -1, best_piece = -1, best_value = 1 << 30;

  const Pieces first = us == white ? P : p;
  const Pieces last = us == white ? K : k;
  for(Pieces piece{ first }; piece <= last; ++piece) {
    u64 bitboard = bb[piece];
    while(bitboard) {
      const Squares from = get_ls1b_index(bitboard);
      if(from != target && piece_reaches(piece, from, target, occupancy)) {
        const int value = material_cp(piece);
        if(value < best_value) {
          best_value = value;
          best_piece = piece;
          best_from = from;
        }
      }
      pop_bit(bitboard, from);
    }
  }

  return best_piece >= 0 ? (best_piece << 8) | best_from : -1;
}

int see_swap(const Squares target, Bitboards& bb, u64& own_white, u64& own_black, const Colors us) {
  const u64 target_mask = one << target;
  int victim = -1;
  for(int piece{ }; piece < PieceCount::all; ++piece) {
    if(bb[piece] & target_mask) {
      victim = piece;
      break;
    }
  }

  if(victim < 0 || victim == K || victim == k)
    return 0;

  const int victim_value = material_cp(victim);
  const int lva = find_lva(us, bb, own_white | own_black, target);
  if(lva < 0)
    return 0;

  const Squares from = to_square(lva & 0x3f);
  const Pieces attacker = to_piece(lva >> 8);

  pop_bit(bb[attacker], from);
  own_white &= ~(one << from);
  own_black &= ~(one << from);
  pop_bit(bb[victim], target);
  if(piece_color(to_piece(victim)) == white)
    own_white &= ~target_mask;
  else
    own_black &= ~target_mask;
  set_bit(bb[attacker], target);
  if(piece_color(attacker) == white)
    own_white |= target_mask;
  else
    own_black |= target_mask;

  // Stand pat: the side to move may decline the exchange, so this capture is never
  // worth less than not capturing at all.
  const int gain = victim_value - see_swap(target, bb, own_white, own_black, opponent(us));
  return gain > 0 ? gain : 0;
}

int see_victim_value(const BoardState& state, const int move) {
  const Squares to = Move::get_move_target(move);
  const Squares victim_square = Move::get_move_enpassant(move) ? (state.side == white ? to + BoardGeometry::ranks : to - BoardGeometry::ranks) : to;

  for(int piece{ }; piece < PieceCount::all; ++piece) {
    if(get_bit(state.bitboards[piece], victim_square))
      return material_cp(piece);
  }
  return 0;
}

} // namespace

int see_evaluate(Board& board, const int move) {
  if(!Move::get_move_capture(move))
    return 0;

  const int first_victim = see_victim_value(board.state, move);
  if(!board.make_move(move, TypeMove::all_moves))
    return 0;

  Bitboards bb = board.state.bitboards;
  u64 own_white = board.state.occupancies[white];
  u64 own_black = board.state.occupancies[black];
  const Colors side_to_move = board.state.side;
  board.pop_state();

  return first_victim - see_swap(Move::get_move_target(move), bb, own_white, own_black, side_to_move);
}

} // namespace maharajah
