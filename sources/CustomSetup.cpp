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

// indices into `generator_pieces` of the compound pieces (A, C, M)
constexpr std::array<int, 3> compound_piece_indices{ 5, 6, 7 };

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

int free_squares_in_row(const BoardState& state, const int row) {
  int free_squares{ };
  for(int file{ }; file < 8; ++file) {
    if(!is_occupied(state, row * 8 + file))
      ++free_squares;
  }
  return free_squares;
}

bool row_is_full(const BoardState& state, const int row) {
  return free_squares_in_row(state, row) == 0;
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

// cheapest non-pawn, i.e. the least a home-rank square can cost once pawns are ruled out there
constexpr int cheapest_piece_weight() {
  int cheapest = generator_pieces[1].weight;
  for(std::size_t index{ 2 }; index < generator_pieces.size(); ++index) {
    if(generator_pieces[index].weight < cheapest)
      cheapest = generator_pieces[index].weight;
  }
  return cheapest;
}

// The budget is spent home rank first, and a piece there outscores a pawn by roughly
// eight to one, so without a reserve the whole budget burns on the back rank and the
// army starts nearly pawnless. Two rules keep a pawn wall:
//   - on the home rank a piece may be bought only while the budget left still covers
//     the pawns this army still wants plus the cheapest piece for every home square
//     still empty;
//   - past the home rank only pawns are bought until the army holds `pawn_target`.
// Pawns are always allowed, so the placement loop always has a candidate.
bool candidate_is_allowed(const int piece_index, const int row_depth, const int remaining_budget, const int slots_before,
    const int row_slots_left, const int pawn_count, const int pawn_target) {
  const int weight = generator_pieces[piece_index].weight;
  const int remaining_after = remaining_budget - weight;

  // the piece must fit the budget, and what is left must fit the free squares
  if(weight > remaining_budget || remaining_after > slots_before - 1)
    return false;

  if(piece_index == 0)
    return true;

  if(row_depth == 0) {
    const int pawns_wanted = pawn_target > pawn_count ? pawn_target - pawn_count : 0;
    return remaining_after >= pawns_wanted + cheapest_piece_weight() * (row_slots_left - 1);
  }

  return pawn_count >= pawn_target;
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
  // pawns this army wants before it spends anything past its home rank
  const int pawn_target = 5 + std::rand() % 4;

  while(remaining_budget > 0) {
    const int active_row = first_open_row(state, player);
    if(active_row < 0)
      return false;

    const int row_depth = player == white ? 7 - active_row : active_row;
    const int row_slots_left = free_squares_in_row(state, active_row);
    const int slots_before = total_remaining_slots(state, player);

    // calls `visit(square, piece_index, score)` for every candidate placement in order
    const auto for_each_candidate = [&](const auto& visit) {
      for(int file{ }; file < 8; ++file) {
        const int square = active_row * 8 + file;
        if(is_occupied(state, square))
          continue;

        for(int index{ }; index < static_cast<int>(generator_pieces.size()); ++index) {
          if(!candidate_is_allowed(index, row_depth, remaining_budget, slots_before, row_slots_left, piece_counts[0], pawn_target))
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

  // both sides play variant rules and so give up castling; an army without a compound
  // piece would carry that handicap for nothing, so reject it and draw again
  for(const int index : compound_piece_indices) {
    if(piece_counts[index] > 0)
      return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// training starts

// a side's own half: 31 squares beside the king
constexpr int max_army_squares{ 31 };

int board_row(const Colors player, const int depth) {
  return player == white ? 7 - depth : depth;
}

// a random free square on a row of the side's half, -1 if the row is full
int random_free_square(const BoardState& state, const Colors player, const int depth) {
  const int row = board_row(player, depth);
  const int free_squares = free_squares_in_row(state, row);
  if(free_squares == 0)
    return -1;
  int skip = std::rand() % free_squares;
  for(int file{ }; file < 8; ++file) {
    const int square = row * 8 + file;
    if(is_occupied(state, square))
      continue;
    if(skip-- == 0)
      return square;
  }
  return -1;
}

void put(BoardState& state, const Colors player, const int piece_index, const int square) {
  const GeneratorPiece& piece = generator_pieces[piece_index];
  set_bit(state.bitboards[player == white ? piece.white_piece : piece.black_piece], to_square(square));
}

// half the armies draw 0-27 pawns evenly, a quarter 0-4, a quarter 16-27
int wide_pawn_count() {
  switch(std::rand() % 4) {
  case 0:
    return std::rand() % 5;
  case 1:
    return 16 + std::rand() % 12;
  default:
    return std::rand() % 28;
  }
}

// An army of any shape: the pawn count first, then pieces until the budget is spent,
// half the time with one favourite piece type taken on four draws of five while it
// fits (knight swarms, several queens); the 1-2 points no piece fits become pawns. Pieces take the
// home rank in random files and spill forward; pawns go mostly to the second rank.
bool generate_wide_side(BoardState& state, const Colors player) {
  std::array<int, 8> counts{ };
  counts[0] = wide_pawn_count();
  const int favourite = std::rand() % 2 == 0 ? 1 + std::rand() % 7 : -1;
  int remaining = army_budget - counts[0];

  while(remaining >= cheapest_piece_weight()) {
    if(favourite > 0 && generator_pieces[favourite].weight <= remaining && std::rand() % 5 != 0) {
      ++counts[favourite];
      remaining -= generator_pieces[favourite].weight;
      continue;
    }
    int fitting{ };
    for(int index{ 1 }; index < static_cast<int>(generator_pieces.size()); ++index)
      fitting += generator_pieces[index].weight <= remaining;
    int roll = std::rand() % fitting;
    for(int index{ 1 }; index < static_cast<int>(generator_pieces.size()); ++index) {
      if(generator_pieces[index].weight <= remaining && roll-- == 0) {
        ++counts[index];
        remaining -= generator_pieces[index].weight;
        break;
      }
    }
  }
  counts[0] += remaining;

  int squares_needed{ };
  for(const int count : counts)
    squares_needed += count;
  if(squares_needed > max_army_squares)
    return false;

  // pieces: home rank first, then forward
  for(int index{ 1 }; index < static_cast<int>(generator_pieces.size()); ++index) {
    for(int placed{ }; placed < counts[index]; ++placed) {
      int square = -1;
      for(int depth{ }; depth < 4 && square < 0; ++depth)
        square = random_free_square(state, player, depth);
      if(square < 0)
        return false;
      put(state, player, index, square);
    }
  }

  // pawns: ranks 2 / 3 / 4 weighted 6 / 3 / 1 among the rows with room, the home rank last
  constexpr std::array<int, 4> pawn_row_weights{ 0, 6, 3, 1 };
  for(int placed{ }; placed < counts[0]; ++placed) {
    int total_weight{ };
    for(int depth{ 1 }; depth < 4; ++depth) {
      if(free_squares_in_row(state, board_row(player, depth)) > 0)
        total_weight += pawn_row_weights[depth];
    }
    int depth{ };
    if(total_weight > 0) {
      int roll = std::rand() % total_weight;
      for(depth = 1; depth < 4; ++depth) {
        if(free_squares_in_row(state, board_row(player, depth)) == 0)
          continue;
        roll -= pawn_row_weights[depth];
        if(roll < 0)
          break;
      }
    }
    const int square = random_free_square(state, player, depth);
    if(square < 0)
      return false;
    put(state, player, 0, square);
  }
  return true;
}

// the orthodox army: home rank beside the king on e1 / e8, pawns in front
void put_orthodox_side(BoardState& state, const Colors player) {
  constexpr std::array<int, 8> home_rank{ 3, 1, 2, 4, -1, 2, 1, 3 };
  for(int file{ }; file < 8; ++file) {
    if(home_rank[file] >= 0)
      put(state, player, home_rank[file], board_row(player, 0) * 8 + file);
    put(state, player, 0, board_row(player, 1) * 8 + file);
  }
}

// a quarter generate_custom_position's kind, the rest any shape
bool generate_training_army(BoardState& state, const Colors player) {
  return std::rand() % 4 == 0 ? generate_side(state, player) : generate_wide_side(state, player);
}

} // namespace

bool generate_training_position(Board& board, const TrainingStart start, const unsigned int seed) {
  std::srand(seed);

  for(int attempt{ }; attempt < max_attempts; ++attempt) {
    (void)board.parse_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    BoardState& state = board.state;
    state.castle = 0;

    if(start == TrainingStart::mixed) {
      const Colors orthodox = std::rand() % 2 == 0 ? white : black;
      const Colors variant = orthodox == white ? black : white;
      put_orthodox_side(state, orthodox);
      const bool built = std::rand() % 2 == 0 ? generate_side(state, variant) : generate_wide_side(state, variant);
      if(!built)
        continue;
      state.castle = orthodox == white ? (wk | wq) : (bk | bq);
      state.side_variant = { variant == white, variant == black };
    } else {
      if(!generate_training_army(state, white) || !generate_training_army(state, black))
        continue;
      state.side_variant = { true, true };
    }

    state.side = static_cast<Colors>(std::rand() % 2);
    state.en_passant = no_square;
    state.halfmove = 0;
    state.standard_rules = !state.has_compound_pieces();
    // every pawn of a fresh variant army is unmoved
    state.pawn_unmoved = state.variant_pawns();
    board.update_occupancies();
    state.hash_key = generate_hash_key(state);

    if(!board.in_check() && board.has_legal_move())
      return true;
  }

  return false;
}

bool generate_custom_position(Board& board, int side_to_move, const unsigned int seed) {
  std::srand(seed);

  if(side_to_move != white && side_to_move != black)
    side_to_move = std::rand() % 2;

  for(int attempt{ }; attempt < max_attempts; ++attempt) {
    (void)board.parse_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    BoardState& state = board.state;

    if(!generate_side(state, white) || !generate_side(state, black))
      continue;

    state.side = static_cast<Colors>(side_to_move);
    state.castle = 0;
    state.en_passant = no_square;
    state.halfmove = 0;
    state.standard_rules = false;
    state.side_variant = { true, true };
    // every pawn of a fresh army is unmoved
    state.pawn_unmoved = state.variant_pawns();
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
  // placement and side to move, the fixed fields of a fresh variant game, then the
  // unmoved pawns (field 8)
  return fen.substr(0, fen.find(' ') + 2) + " - - 0 1 Vv" + fen.substr(fen.rfind(' '));
}

} // namespace maharajah
