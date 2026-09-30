#include "../headers/CustomSetup.h"
#include "../headers/Bitboard.h"
#include "../headers/Zobrist.h"

#include <cstdlib>

namespace maharajah {

namespace {

struct GeneratorPiece {
  Pieces white_piece;
  Pieces black_piece;
  int weight;
};

constexpr std::array<GeneratorPiece, 8> generator_pieces{ {
    { P, p, 1 },
    { N, n, 3 },
    { B, b, 3 },
    { R, r, 5 },
    { Q, q, 9 },
    { A, a, 6 },
    { C, c, 8 },
    { M, m, 13 },
} };

constexpr std::array<int, 8> preferred_piece_counts{ 8, 2, 2, 2, 1, 1, 1, 1 };

// how well each piece suits a row, by distance from the side's back rank
constexpr std::array<std::array<int, 4>, 8> placement_scores{ {
    { 2, 8, 16, 22 },
    { 12, 16, 12, 7 },
    { 12, 15, 11, 6 },
    { 18, 11, 5, 2 },
    { 17, 9, 3, 1 },
    { 15, 11, 6, 2 },
    { 16, 10, 4, 1 },
    { 14, 8, 2, 1 },
} };

constexpr int army_budget{ 39 };
constexpr int max_attempts{ 5000 };

bool is_occupied(const BoardState& state, const int square) {
  for(const u64 bitboard : state.bitboards) {
    if(get_bit(bitboard, to_square(square)))
      return true;
  }
  return false;
}

bool row_is_full(const BoardState& state, const int row) {
  for(int file{ }; file < 8; ++file) {
    if(!is_occupied(state, row * 8 + file))
      return false;
  }
  return true;
}

// white fills rows 7..4 (ranks 1..4), black rows 0..3 (ranks 8..5)
int first_open_row(const BoardState& state, const Colors player) {
  if(player == white) {
    for(int row{ 7 }; row >= 4; --row) {
      if(!row_is_full(state, row))
        return row;
    }
    return -1;
  }

  for(int row{ 0 }; row <= 3; ++row) {
    if(!row_is_full(state, row))
      return row;
  }
  return -1;
}

int total_remaining_slots(const BoardState& state, const Colors player) {
  const int first_row = player == white ? 4 : 0;
  int remaining{ };

  for(int row{ first_row }; row < first_row + 4; ++row) {
    for(int file{ }; file < 8; ++file) {
      if(!is_occupied(state, row * 8 + file))
        ++remaining;
    }
  }

  return remaining;
}

int candidate_score(
    const int piece_index, const int file, const int height, const std::array<int, 8>& piece_counts, const int remaining_budget, const int remaining_slots) {
  const int remaining_after = remaining_budget - generator_pieces[piece_index].weight;
  const int centrality_bonus = 3 - std::abs(3 - file);
  int score = placement_scores[piece_index][height];

  if(piece_counts[piece_index] >= preferred_piece_counts[piece_index])
    score -= 8 * (piece_counts[piece_index] - preferred_piece_counts[piece_index] + 1);

  if(height == 0 && file != 4)
    score += 2;

  // knights, bishops and archbishops like the centre
  if(piece_index == 1 || piece_index == 2 || piece_index == 5)
    score += centrality_bonus;

  // fill the last slots with pawns
  if(piece_index == 0 && remaining_slots <= 6)
    score += 4;

  if(remaining_after == 0)
    score += 10;

  if(remaining_after > 0 && remaining_after < 3 && piece_index != 0)
    score -= 5;

  return score > 0 ? score : 1;
}

bool generate_side(BoardState& state, const Colors player) {
  std::array<int, 8> piece_counts{ };
  int remaining_budget{ army_budget };

  while(remaining_budget > 0) {
    const int active_row = first_open_row(state, player);
    if(active_row < 0)
      return false;

    const int row_depth = player == white ? 7 - active_row : active_row;
    const int slots_before = total_remaining_slots(state, player);

    // calls `visit(square, piece_index, score)` for every candidate placement in order
    const auto for_each_candidate = [&](const auto& visit) {
      for(int file{ }; file < 8; ++file) {
        const int square = active_row * 8 + file;
        if(is_occupied(state, square))
          continue;

        for(int index{ }; index < static_cast<int>(generator_pieces.size()); ++index) {
          const int weight = generator_pieces[index].weight;
          // the piece must fit the budget, and what is left must fit the free squares
          if(weight > remaining_budget || remaining_budget - weight > slots_before - 1)
            continue;

          if(!visit(square, index, candidate_score(index, file, row_depth, piece_counts, remaining_budget, slots_before)))
            return;
        }
      }
    };

    int total_score{ };
    for_each_candidate([&](int, int, const int score) {
      total_score += score;
      return true;
    });

    if(total_score <= 0)
      return false;

    int roll = std::rand() % total_score;
    int chosen_square = -1;
    int chosen_piece_index = -1;

    for_each_candidate([&](const int square, const int index, const int score) {
      if(roll < score) {
        chosen_square = square;
        chosen_piece_index = index;
        return false;
      }
      roll -= score;
      return true;
    });

    if(chosen_square < 0)
      return false;

    const GeneratorPiece& selected = generator_pieces[chosen_piece_index];
    set_bit(state.bitboards[player == white ? selected.white_piece : selected.black_piece], to_square(chosen_square));
    ++piece_counts[chosen_piece_index];
    remaining_budget -= selected.weight;
  }

  return true;
}

} // namespace

bool generate_custom_position(Board& board, int side_to_move, const unsigned int seed) {
  std::srand(seed);

  if(side_to_move != white && side_to_move != black)
    side_to_move = std::rand() % 2;

  for(int attempt{ }; attempt < max_attempts; ++attempt) {
    board.parse_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    BoardState& state = board.state;

    if(!generate_side(state, white) || !generate_side(state, black))
      continue;

    state.side = static_cast<Colors>(side_to_move);
    state.castle = 0;
    state.en_passant = no_square;
    state.halfmove = 0;
    state.standard_rules = false;
    state.side_variant = { true, true };
    board.update_occupancies();
    state.hash_key = generate_hash_key(state);

    // the side to move must not start in check, and must have a move
    if(!board.in_check() && board.has_legal_move())
      return true;
  }

  return false;
}

std::string custom_position_fen(const Board& board) {
  const std::string fen = board.to_fen();
  // placement and side to move, then the fixed fields of a fresh variant game
  return fen.substr(0, fen.find(' ') + 2) + " - - 0 1 Vv";
}

} // namespace maharajah
