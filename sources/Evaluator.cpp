#include "../headers/Evaluator.h"
#include "../headers/Bitboard.h"

#include <cstdlib>

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
  return std::abs(Evaluation::material_score[opening][piece]);
}

int attacker_count_on_square(const BoardState& state, const Squares square, const Colors side) {
  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];
  const bool is_white = side == white;
  int attackers{ };

  attackers += count_bits(attack_tables.pawn[opponent(side)][square] & bb[is_white ? P : p]);
  attackers += count_bits(attack_tables.knight[square] & bb[is_white ? N : n]);
  attackers += count_bits(get_bishop_attacks(square, occupancy) & bb[is_white ? B : b]);
  attackers += count_bits(get_archbishop_attacks(square, occupancy) & bb[is_white ? A : a]);
  attackers += count_bits(get_rook_attacks(square, occupancy) & bb[is_white ? R : r]);
  attackers += count_bits(get_chancellor_attacks(square, occupancy) & bb[is_white ? C : c]);
  attackers += count_bits(get_queen_attacks(square, occupancy) & bb[is_white ? Q : q]);
  attackers += count_bits(get_amazon_attacks(square, occupancy) & bb[is_white ? M : m]);
  attackers += count_bits(attack_tables.king[square] & bb[is_white ? K : k]);

  return attackers;
}

int least_attacker_value_on_square(const BoardState& state, const Squares square, const Colors side) {
  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];
  const bool is_white = side == white;

  if(attack_tables.pawn[opponent(side)][square] & bb[is_white ? P : p])
    return piece_material_value(P);
  // cheapest first: N < B < A < R < C < Q < M < K
  for(const Pieces piece : { N, B, A, R, C, Q, M, K }) {
    const Pieces own = is_white ? piece : to_piece(piece + PieceCount::per_side);
    if(get_piece_attacks(own, square, occupancy) & bb[own])
      return piece_material_value(own);
  }

  return 0;
}

// Pieces with the same material class share the penalty divisors.
enum class SafetyClass { heavy, rook, minor, pawn };

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
  const SafetyClass kind = safety_class(piece);
  int penalty{ };

  switch(kind) {
  case SafetyClass::heavy:
    penalty = defended ? material_value / 10 : material_value / 4;
    break;
  case SafetyClass::rook:
    penalty = defended ? material_value / 12 : material_value / 5;
    break;
  case SafetyClass::minor:
    penalty = defended ? material_value / 14 : material_value / 6;
    break;
  case SafetyClass::pawn:
    penalty = defended ? material_value / 18 : material_value / 10;
    break;
  }

  if(enemy_attackers > own_defenders)
    penalty += (material_value * (enemy_attackers - own_defenders)) / (kind == SafetyClass::heavy ? 14 : 18);

  if(!defended)
    penalty += material_value / 12;

  if(least_attacker_value > 0 && least_attacker_value < material_value) {
    penalty += (material_value - least_attacker_value) / 10;

    if(!defended)
      penalty += (material_value - least_attacker_value) / 12;
    else if(enemy_attackers >= own_defenders)
      penalty += (material_value - least_attacker_value) / 16;
  }

  int cap{ };
  switch(kind) {
  case SafetyClass::heavy:
    cap = (material_value * 2) / 5;
    break;
  case SafetyClass::rook:
    cap = material_value / 3;
    break;
  case SafetyClass::minor:
    cap = material_value / 4;
    break;
  case SafetyClass::pawn:
    cap = material_value / 5;
    break;
  }

  return penalty > cap ? cap : penalty;
}

int Evaluator::game_phase_score(const BoardState& state) {
  int score{ };

  for(const Pieces piece : { N, B, R, Q, A, C, M }) {
    score += count_bits(state.bitboards[piece]) * Evaluation::material_score[opening][piece];
    const Pieces black_piece = to_piece(piece + PieceCount::per_side);
    score += count_bits(state.bitboards[black_piece]) * -Evaluation::material_score[opening][black_piece];
  }

  return score;
}

// position evaluation
int Evaluator::evaluate(const BoardState& state, const EvalConfig& config) {
  const int phase_score = game_phase_score(state);
  Phase game_phase;

  if(phase_score > Evaluation::opening_phase_score)
    game_phase = opening;
  else if(phase_score < Evaluation::endgame_phase_score)
    game_phase = endgame;
  else
    game_phase = middlegame;

  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];
  const u64 all_pawns = bb[P] | bb[p];
  int score_opening{ }, score_endgame{ };
  std::array<int, 2> bishops{ };

  for(Pieces piece{ P }; piece <= k; ++piece) {
    const Colors color = piece_color(piece);
    const int sign = color == white ? 1 : -1;
    const Pieces own_pawn = color == white ? P : p;
    const Pieces enemy_pawn = color == white ? p : P;
    u64 bitboard = bb[piece];

    while(bitboard) {
      const Squares square = get_ls1b_index(bitboard);
      const int index = color == white ? square : Evaluation::mirror_score[square];
      // everything below is from the piece owner's point of view
      int opening_term{ }, endgame_term{ };

      const auto add_positional = [&](const PieceKind kind) {
        opening_term += Evaluation::positional_score[opening][kind][index];
        endgame_term += Evaluation::positional_score[endgame][kind][index];
      };

      const auto add_mobility = [&](const int squares, const int unit, const int weight_opening, const int weight_endgame) {
        opening_term += (squares - unit) * weight_opening;
        endgame_term += (squares - unit) * weight_endgame;
      };

      const auto add_both = [&](const int value) {
        opening_term += value;
        endgame_term += value;
      };

      const auto subtract_safety_penalty = [&] {
        const int penalty = piece_safety_penalty(state, piece, square);
        opening_term -= penalty;
        endgame_term -= penalty;
      };

      score_opening += Evaluation::material_score[opening][piece];
      score_endgame += Evaluation::material_score[endgame][piece];

      switch(piece) {
      case P:
      case p: {
        add_positional(pawn_kind);

        const int double_pawns = count_bits(bb[own_pawn] & eval_masks.file[square]);
        if(double_pawns > 1) {
          opening_term += (double_pawns - 1) * Evaluation::double_pawn_penalty_opening;
          endgame_term += (double_pawns - 1) * Evaluation::double_pawn_penalty_endgame;
        }

        if((bb[own_pawn] & eval_masks.isolated[square]) == 0) {
          opening_term += Evaluation::isolated_pawn_penalty_opening;
          endgame_term += Evaluation::isolated_pawn_penalty_endgame;
        }

        if((eval_masks.passed[color][square] & bb[enemy_pawn]) == 0)
          add_both(Evaluation::passed_pawn_bonus[Evaluation::rank_of(index)]);

        subtract_safety_penalty();
        break;
      }
      case N:
      case n:
        add_positional(knight_kind);
        add_mobility(count_bits(attack_tables.knight[square] & ~state.occupancies[color]), 4, config.knight_mobility_opening, config.knight_mobility_endgame);
        subtract_safety_penalty();
        break;
      case B:
      case b:
        ++bishops[color];
        add_positional(bishop_kind);
        add_mobility(count_bits(get_bishop_attacks(square, occupancy)),
                     Evaluation::bishop_unit,
                     Evaluation::bishop_mobility_opening,
                     Evaluation::bishop_mobility_endgame);
        subtract_safety_penalty();
        break;
      case R:
      case r:
        add_positional(rook_kind);
        add_mobility(count_bits(get_rook_attacks(square, occupancy)), 7, config.rook_mobility_opening, config.rook_mobility_endgame);

        if((bb[own_pawn] & eval_masks.file[square]) == 0)
          add_both(Evaluation::semi_open_file_score);

        if((all_pawns & eval_masks.file[square]) == 0)
          add_both(Evaluation::open_file_score);

        subtract_safety_penalty();
        break;
      case Q:
      case q:
        add_positional(queen_kind);
        add_mobility(
            count_bits(get_queen_attacks(square, occupancy)), Evaluation::queen_unit, Evaluation::queen_mobility_opening, Evaluation::queen_mobility_endgame);
        subtract_safety_penalty();
        break;
      case K:
      case k:
        add_positional(king_kind);

        if((bb[own_pawn] & eval_masks.file[square]) == 0)
          add_both(-Evaluation::semi_open_file_score);

        if((all_pawns & eval_masks.file[square]) == 0)
          add_both(-Evaluation::open_file_score);

        add_both(count_bits(attack_tables.king[square] & state.occupancies[color]) * Evaluation::king_shield_bonus);
        break;
      case A:
      case a:
        // compound pieces: positional value is the sum of their components
        add_positional(bishop_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_archbishop_attacks(square, occupancy)),
                     Evaluation::bishop_unit + 4,
                     Evaluation::bishop_mobility_opening,
                     Evaluation::bishop_mobility_endgame);
        subtract_safety_penalty();
        break;
      case C:
      case c:
        add_positional(rook_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_chancellor_attacks(square, occupancy)),
                     Evaluation::queen_unit - 1,
                     Evaluation::queen_mobility_opening,
                     Evaluation::queen_mobility_endgame);
        subtract_safety_penalty();
        break;
      case M:
      case m:
        add_positional(queen_kind);
        add_positional(knight_kind);
        add_mobility(count_bits(get_amazon_attacks(square, occupancy)),
                     Evaluation::queen_unit + 4,
                     Evaluation::queen_mobility_opening,
                     Evaluation::queen_mobility_endgame);
        subtract_safety_penalty();
        break;
      default:
        break;
      }

      score_opening += sign * opening_term;
      score_endgame += sign * endgame_term;

      pop_bit(bitboard, square);
    }
  }

  if(bishops[white] >= 2) {
    score_opening += config.bishop_pair_bonus_opening;
    score_endgame += config.bishop_pair_bonus_endgame;
  }

  if(bishops[black] >= 2) {
    score_opening -= config.bishop_pair_bonus_opening;
    score_endgame -= config.bishop_pair_bonus_endgame;
  }

  int score{ };
  if(game_phase == middlegame)
    score = (score_opening * phase_score + score_endgame * (Evaluation::opening_phase_score - phase_score)) / Evaluation::opening_phase_score;
  else if(game_phase == opening)
    score = score_opening;
  else
    score = score_endgame;

  score += state.side == white ? config.tempo_bonus : -config.tempo_bonus;

  // return final evaluation based on side
  return (state.side == white) ? score : -score;
}

} // namespace maharajah
