#include "../headers/Evaluator.h"
#include "../headers/Bitboard.h"
#include "../headers/Evaluation.h"

#include <algorithm>
#include <cstddef>

namespace maharajah {

namespace {

struct EvalMasks {
  std::array<u64, 64> file{ };
  std::array<u64, 64> isolated{ };
  // squares in front of a pawn on its own and the adjacent files
  std::array<std::array<u64, 64>, 2> passed{ };
};

constexpr u64 file_mask(const int file) {
  u64 mask{ };
  if(file < 0 || file > 7)
    return mask;
  for(int row{ }; row < 8; ++row)
    mask |= one << (row * 8 + file);
  return mask;
}

constexpr u64 row_mask(const int row) {
  return u64{ 0xFF } << (row * 8);
}

constexpr EvalMasks make_eval_masks() {
  EvalMasks masks{ };

  for(int row{ }; row < 8; ++row) {
    for(int file{ }; file < 8; ++file) {
      const int square = row * 8 + file;
      const u64 neighbours = file_mask(file - 1) | file_mask(file + 1);

      masks.file[square] = file_mask(file);
      masks.isolated[square] = neighbours;
      masks.passed[white][square] = neighbours | file_mask(file);
      masks.passed[black][square] = neighbours | file_mask(file);

      // white pawns advance towards row 0, black pawns towards row 7
      for(int other{ row }; other < 8; ++other)
        masks.passed[white][square] &= ~row_mask(other);
      for(int other{ 0 }; other <= row; ++other)
        masks.passed[black][square] &= ~row_mask(other);
    }
  }

  return masks;
}

constexpr EvalMasks eval_masks = make_eval_masks();

int piece_material_value(const Pieces piece) {
  return Evaluation::piece_value[piece];
}

int attacker_count_on_square(const BoardState& state, const Squares square, const Colors side) {
  const u64 occupancy = state.occupancies[both];
  const AttackerGroups attackers(state.bitboards, side);

  // the groups' attack sets never overlap, so a compound piece reaching the square is
  // counted once, in the set its move falls in
  return count_bits(attack_tables.pawn[opponent(side)][square] & attackers.pawns) + count_bits(attack_tables.knight[square] & attackers.knight_movers) +
         count_bits(get_bishop_attacks(square, occupancy) & attackers.diagonal_movers) +
         count_bits(get_rook_attacks(square, occupancy) & attackers.line_movers) + count_bits(attack_tables.king[square] & attackers.king);
}

int least_attacker_value_on_square(const BoardState& state, const Squares square, const Colors side) {
  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];

  if(attack_tables.pawn[opponent(side)][square] & bb[colored(P, side)])
    return piece_material_value(P);
  // cheapest first: N < B < A < R < C < Q < M < K
  for(const Pieces piece : { N, B, A, R, C, Q, M, K }) {
    const Pieces own = colored(piece, side);
    if(get_piece_attacks(own, square, occupancy) & bb[own])
      return piece_material_value(own);
  }

  return 0;
}

// Pieces with the same material class share the penalty divisors.
enum class SafetyClass { heavy, rook, minor, pawn };

// The piece safety penalty of a class, as divisors of the piece's material value.
struct SafetyDivisors {
  // base penalty for an attacked piece
  int defended;
  int undefended;
  // per attacker more than defenders
  int outnumbered;
  // the penalty is capped at value * cap_numerator / cap_denominator
  int cap_numerator;
  int cap_denominator;
};

// indexed by SafetyClass
constexpr std::array<SafetyDivisors, 4> safety_divisors{ {
    { 10, 4, 14, 2, 5 }, // heavy
    { 12, 5, 18, 1, 3 }, // rook
    { 14, 6, 18, 1, 4 }, // minor
    { 18, 10, 18, 1, 5 }, // pawn
} };

SafetyClass safety_class(const Pieces piece) {
  switch(piece) {
  case Q:
  case q:
  case M:
  case m:
    return SafetyClass::heavy;
  case R:
  case r:
  case C:
  case c:
    return SafetyClass::rook;
  case P:
  case p:
    return SafetyClass::pawn;
  default:
    return SafetyClass::minor;
  }
}

} // namespace

int Evaluator::piece_safety_penalty(const BoardState& state, const Pieces piece, const Squares square) {
  if(piece == K || piece == k)
    return 0;

  const Colors own_side = piece_color(piece);
  const Colors enemy_side = opponent(own_side);
  const int enemy_attackers = attacker_count_on_square(state, square, enemy_side);

  if(enemy_attackers == 0)
    return 0;

  const int own_defenders = attacker_count_on_square(state, square, own_side);
  const bool defended = own_defenders > 0;
  const int material_value = piece_material_value(piece);
  const int least_attacker_value = least_attacker_value_on_square(state, square, enemy_side);
  const SafetyDivisors& divisors = safety_divisors[static_cast<std::size_t>(safety_class(piece))];

  int penalty = material_value / (defended ? divisors.defended : divisors.undefended);

  if(enemy_attackers > own_defenders)
    penalty += (material_value * (enemy_attackers - own_defenders)) / divisors.outnumbered;

  if(!defended)
    penalty += material_value / 12;

  // attacked by a cheaper piece, which would win material by capturing
  if(least_attacker_value > 0 && least_attacker_value < material_value) {
    const int exchange_gain = material_value - least_attacker_value;
    penalty += exchange_gain / 10;

    if(!defended)
      penalty += exchange_gain / 12;
    else if(enemy_attackers >= own_defenders)
      penalty += exchange_gain / 16;
  }

  return std::min(penalty, material_value * divisors.cap_numerator / divisors.cap_denominator);
}

int Evaluator::game_phase_score(const BoardState& state) {
  int score{ };

  for(const Pieces piece : { N, B, R, Q, A, C, M }) {
    score += (count_bits(state.bitboards[piece]) + count_bits(state.bitboards[colored(piece, black)])) * Evaluation::piece_value[piece];
  }

  return score;
}

namespace {

constexpr auto opening_flat = flat_weights(opening);
constexpr auto endgame_flat = flat_weights(endgame);

// position of `weight`, a member of the opening weights, in the flat array
std::size_t weight_index(const int& weight) {
  const auto offset = reinterpret_cast<const std::byte*>(&weight) - reinterpret_cast<const std::byte*>(&Evaluation::weights[opening]);
  return static_cast<std::size_t>(offset) / sizeof(int);
}

// The classic evaluation from white's point of view, before the tempo bonus. With
// `traced`, also counts how often each weight enters the score (see EvalTrace).
template <bool traced>
int white_score(const BoardState& state, EvalTrace* trace) {
  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];
  const u64 all_pawns = bb[P] | bb[p];
  const EvalWeights& opening_weights = Evaluation::weights[opening];
  int score_opening{ }, score_endgame{ };
  std::array<int, 2> bishops{ };

  for(Pieces piece{ P }; piece <= k; ++piece) {
    const Colors color = piece_color(piece);
    const int sign = color == white ? 1 : -1;
    const int kind = color == white ? piece : piece - PieceCount::per_side;
    const Pieces own_pawn = colored(P, color);
    const Pieces enemy_pawn = colored(P, opponent(color));
    u64 bitboard = bb[piece];

    while(bitboard) {
      const Squares square = pop_ls1b(bitboard);
      const int index = color == white ? square : Evaluation::mirror_score[square];
      // everything below is from the piece owner's point of view
      int opening_term{ }, endgame_term{ };

      // `weight` is a member of the opening weights; its endgame twin sits at the same offset
      const auto add = [&](const int& weight, const int count) {
        const std::size_t offset = weight_index(weight);
        opening_term += count * opening_flat[offset];
        endgame_term += count * endgame_flat[offset];
        if constexpr(traced)
          trace->coefficients[offset] += sign * count;
      };

      const auto add_positional = [&](const PieceKind positional_kind) { add(opening_weights.positional[positional_kind][index], 1); };

      const auto add_mobility = [&](const int squares, const int unit, const int& weight) { add(weight, squares - unit); };

      const auto subtract_safety_penalty = [&] {
        const int penalty = Evaluator::piece_safety_penalty(state, piece, square);
        opening_term -= penalty;
        endgame_term -= penalty;
        if constexpr(traced)
          trace->fixed -= sign * penalty;
      };

      if(kind == K) {
        score_opening += sign * Evaluation::king_material;
        score_endgame += sign * Evaluation::king_material;
        if constexpr(traced)
          trace->fixed += sign * Evaluation::king_material;
      } else {
        add(opening_weights.material[static_cast<std::size_t>(kind)], 1);
      }

      switch(piece) {
      case P:
      case p: {
        add_positional(pawn_kind);

        const int double_pawns = count_bits(bb[own_pawn] & eval_masks.file[square]);
        if(double_pawns > 1)
          add(opening_weights.double_pawn, double_pawns - 1);

        if((bb[own_pawn] & eval_masks.isolated[square]) == 0)
          add(opening_weights.isolated_pawn, 1);

        if((eval_masks.passed[color][square] & bb[enemy_pawn]) == 0)
          add(opening_weights.passed_pawn[static_cast<std::size_t>(Evaluation::rank_of(index))], 1);

        subtract_safety_penalty();
        break;
      }
      case N:
      case n:
        add_positional(knight_kind);
        add_mobility(count_bits(attack_tables.knight[square] & ~state.occupancies[color]), Evaluation::knight_unit, opening_weights.knight_mobility);
        subtract_safety_penalty();
        break;
      case B:
      case b:
        ++bishops[color];
        add_positional(bishop_kind);
        add_mobility(count_bits(get_bishop_attacks(square, occupancy)), Evaluation::bishop_unit, opening_weights.bishop_mobility);
        subtract_safety_penalty();
        break;
      case R:
      case r:
        add_positional(rook_kind);
        add_mobility(count_bits(get_rook_attacks(square, occupancy)), Evaluation::rook_unit, opening_weights.rook_mobility);

        if((bb[own_pawn] & eval_masks.file[square]) == 0)
          add(opening_weights.rook_semi_open_file, 1);

        if((all_pawns & eval_masks.file[square]) == 0)
          add(opening_weights.rook_open_file, 1);

        subtract_safety_penalty();
        break;
      case Q:
      case q:
        add_positional(queen_kind);
        add_mobility(count_bits(get_queen_attacks(square, occupancy)), Evaluation::queen_unit, opening_weights.queen_mobility);
        subtract_safety_penalty();
        break;
      case K:
      case k:
        add_positional(king_kind);

        if((bb[own_pawn] & eval_masks.file[square]) == 0)
          add(opening_weights.king_semi_open_file, 1);

        if((all_pawns & eval_masks.file[square]) == 0)
          add(opening_weights.king_open_file, 1);

        add(opening_weights.king_shield, count_bits(attack_tables.king[square] & state.occupancies[color]));
        break;
      case A:
      case a:
        // compound pieces: positional value is the sum of their components
        add_positional(bishop_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_archbishop_attacks(square, occupancy)), Evaluation::bishop_unit + 4, opening_weights.archbishop_mobility);
        subtract_safety_penalty();
        break;
      case C:
      case c:
        add_positional(rook_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_chancellor_attacks(square, occupancy)), Evaluation::queen_unit - 1, opening_weights.chancellor_mobility);
        subtract_safety_penalty();
        break;
      case M:
      case m:
        add_positional(queen_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_amazon_attacks(square, occupancy)), Evaluation::queen_unit + 4, opening_weights.amazon_mobility);
        subtract_safety_penalty();
        break;
      default:
        break;
      }

      score_opening += sign * opening_term;
      score_endgame += sign * endgame_term;
    }
  }

  for(const Colors color : { white, black }) {
    if(bishops[color] < 2)
      continue;
    const int sign = color == white ? 1 : -1;
    score_opening += sign * opening_weights.bishop_pair;
    score_endgame += sign * Evaluation::weights[endgame].bishop_pair;
    if constexpr(traced)
      trace->coefficients[weight_index(opening_weights.bishop_pair)] += sign;
  }

  // tapered between the opening and endgame scores by the material on the board
  const int phase = Evaluator::phase(state);
  if constexpr(traced)
    trace->phase = phase;
  return (score_opening * phase + score_endgame * (Evaluator::phase_range - phase)) / Evaluator::phase_range;
}

} // namespace

int Evaluator::phase(const BoardState& state) {
  return std::clamp(game_phase_score(state), Evaluation::endgame_phase_score, Evaluation::opening_phase_score) - Evaluation::endgame_phase_score;
}

int Evaluator::evaluate(const BoardState& state) {
  const int score = white_score<false>(state, nullptr) + (state.side == white ? Evaluation::tempo_bonus : -Evaluation::tempo_bonus);
  return state.side == white ? score : -score;
}

int Evaluator::evaluate_white(const BoardState& state, EvalTrace& trace) {
  trace = EvalTrace{ };
  return white_score<true>(state, &trace);
}

} // namespace maharajah
