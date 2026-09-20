#include "../headers/Search.h"
#include "../headers/Bitboard.h"
#include "../headers/Evaluator.h"

#include <algorithm>

namespace maharajah {

SearchResult Search::run(const int depth) {
  nodes_ = 0;
  best_move_ = 0;

  const int score = negamax(-infinity, infinity, depth, 0);

  return SearchResult{ best_move_, score, depth, nodes_ };
}

// fifty-move rule or repetition of an earlier position in the current line
bool Search::is_draw() const {
  const auto& state = board_.state;

  if(state.halfmove >= 100)
    return true;

  // only positions with the same side to move can repeat, and none before the last irreversible move
  const int oldest = std::max(0, board_.ply - state.halfmove);
  for(int i = board_.ply - 2; i >= oldest; i -= 2) {
    if(board_.history[i].same_position(state))
      return true;
  }

  return false;
}

// negamax alpha beta search
int Search::negamax(int alpha, int beta, int depth, int ply) {
  // recurrsion escapre condition
  if(depth == 0)
    // return evaluation
    return quiescence(alpha, beta);

  // increment nodes count
  nodes_++;

  if(ply > 0 && is_draw())
    return 0;

  Colors side = (board_.state.side == white) ? white : black;

  int in_check = board_.is_square_attacked((side == white) ? get_ls1b_index(board_.state.bitboards[K]) : get_ls1b_index(board_.state.bitboards[k]),
                                           (side == white) ? black : white);

  // legal moves counter
  int legal_moves = 0;

  // best move so far
  int best_sofar = 0;

  // old value of alpha
  int old_alpha = alpha;

  // generate moves
  MoveList moves_list;
  board_.generate_moves(moves_list);

  // loop over moves within a movelist
  for(size_t i{ }; i < moves_list.size(); ++i) {
    // preserve board state handled by make_move

    const int move = moves_list[i];
    // make sure to make only legal moves
    if(board_.make_move(move, TypeMove::all_moves) == 0) {
      // make_move handles popping if illegal
      continue;
    }

    // increment legal moves
    legal_moves++;

    // score current move
    int score = -negamax(-beta, -alpha, depth - 1, ply + 1);

    // take move back
    board_.pop_state();

    // fail-hard beta cutoff
    if(score >= beta) {
      // node (move) fails high
      return beta;
    }

    // found a better move
    if(score > alpha) {
      // PV node (move)
      alpha = score;

      // if root move
      if(ply == 0)
        // associate best move with the best score
        best_sofar = move;
    }
  }

  // we don't have any legal moves to make in the current postion
  if(legal_moves == 0) {
    // king is in check
    if(in_check)
      // return mating score (assuming closest distance to mating position)
      return -mate_score + ply;
    // king is not in check
    else
      // return stalemate score
      return 0;
  }

  // found better move
  if(old_alpha != alpha) {
    // init best move
    if(ply == 0)
      best_move_ = best_sofar;
  }

  // node (move) fails low
  return alpha;
}

// quiescence search
int Search::quiescence(int alpha, int beta) {
  nodes_++;

  // evaluate position
  int evaluation = Evaluator::evaluate(board_.state);

  // fail-hard beta cutoff
  if(evaluation >= beta) {
    // node (move) fails high
    return beta;
  }

  // found a better move
  if(evaluation > alpha) {
    // PV node (move)
    alpha = evaluation;
  }

  // generate moves
  MoveList moves_list;
  board_.generate_moves(moves_list);

  // loop over moves within a movelist
  for(size_t i{ }; i < moves_list.size(); ++i) {
    // preserve board state handled by make_move

    const int move = moves_list[i];

    // make sure to make only legal moves
    if(board_.make_move(move, TypeMove::only_captures) == 0) {
      // make_move returns false and does not push state if not capture or illegal
      continue;
    }

    // score current move
    int score = -quiescence(-beta, -alpha);

    board_.pop_state();

    // fail-hard beta cutoff
    if(score >= beta) {
      // node (move) fails high
      return beta;
    }

    // found a better move
    if(score > alpha) {
      // PV node (move)
      alpha = score;
    }
  }

  // node (move) fails low
  return alpha;
}

} // namespace maharajah
