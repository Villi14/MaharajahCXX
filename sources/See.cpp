#include "../headers/See.h"
#include "../headers/Bitboard.h"
#include "../headers/Evaluation.h"
#include "../headers/Move.h"

#include <optional>

namespace maharajah {

namespace {

using Bitboards = std::array<u64, PieceCount::all>;

int material_cp(const int piece) {
  return Evaluation::piece_value[piece];
}

bool piece_reaches(const Pieces piece, const Squares from, const Squares to, const u64 occupancy) {
  const u64 target = one << to;

  if(piece == P)
    return attack_tables.pawn[white][from] & target;
  if(piece == p)
    return attack_tables.pawn[black][from] & target;
  return get_piece_attacks(piece, from, occupancy) & target;
}

struct Attacker {
  Pieces piece;
  Squares square;
};

// the least valuable piece of side `us` attacking `target` (the first one found among equals)
std::optional<Attacker> least_valuable_attacker(const Colors us, const Bitboards& bb, const u64 occupancy, const Squares target) {
  std::optional<Attacker> best;
  int best_value = 1 << 30;

  for(Pieces piece{ colored(P, us) }; piece <= colored(K, us); ++piece) {
    u64 bitboard = bb[piece];
    while(bitboard) {
      const Squares from = pop_ls1b(bitboard);
      if(from != target && piece_reaches(piece, from, target, occupancy)) {
        const int value = material_cp(piece);
        if(value < best_value) {
          best_value = value;
          best = Attacker{ piece, from };
        }
      }
    }
  }

  return best;
}

// Material side `us` wins by capturing on `target` with its least valuable attacker,
// then the other side recapturing the same way, and so on; `bb` and `occupancy` (by
// colour) are updated as the exchange goes on.
int see_swap(const Squares target, Bitboards& bb, std::array<u64, 2>& occupancy, const Colors us) {
  const Pieces victim = find_piece(bb, target);
  if(victim == no_pieces || victim == K || victim == k)
    return 0;

  const auto attacker = least_valuable_attacker(us, bb, occupancy[white] | occupancy[black], target);
  if(!attacker)
    return 0;

  const u64 target_mask = one << target;
  pop_bit(bb[attacker->piece], attacker->square);
  occupancy[us] &= ~(one << attacker->square);
  pop_bit(bb[victim], target);
  occupancy[piece_color(victim)] &= ~target_mask;
  set_bit(bb[attacker->piece], target);
  occupancy[us] |= target_mask;

  // Stand pat: the side to move may decline the exchange, so this capture is never
  // worth less than not capturing at all.
  const int gain = material_cp(victim) - see_swap(target, bb, occupancy, opponent(us));
  return gain > 0 ? gain : 0;
}

int see_victim_value(const BoardState& state, const int move) {
  const Squares to = Move::get_move_target(move);
  const Squares victim_square = Move::get_move_enpassant(move) ? square_behind(to, state.side) : to;
  const Pieces victim = state.piece_on(victim_square);
  return victim == no_pieces ? 0 : material_cp(victim);
}

} // namespace

int see_evaluate(Board& board, const int move) {
  if(!Move::get_move_capture(move))
    return 0;

  const int first_victim = see_victim_value(board.state, move);
  if(!board.make_move(move, TypeMove::all_moves))
    return 0;

  // the exchange goes on from the position after the first capture
  Bitboards bb = board.state.bitboards;
  std::array<u64, 2> occupancy{ board.state.occupancies[white], board.state.occupancies[black] };
  const Colors side_to_move = board.state.side;
  board.pop_state();

  return first_victim - see_swap(Move::get_move_target(move), bb, occupancy, side_to_move);
}

} // namespace maharajah
