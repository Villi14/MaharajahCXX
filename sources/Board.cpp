#include "../headers/Board.h"
#include "../headers/Bitboard.h"

using namespace std;

namespace maharajah {

/**
 * @brief Checks if a given square is attacked by a given side.
 *
 * This function checks if a given square is attacked by a given side.
 * It does this by checking if any of the pieces of the given side can attack the given square.
 * The function first checks if the given square is attacked by pawns, then by knights, then by bishops, then by rooks, then by queens, and finally by kings.
 * If the given square is attacked by any of the pieces of the given side, the function returns true.
 * Otherwise, it returns false.
 *
 * @param square The square to check for attacks.
 * @param side The side to check for attacks.
 * @return true if the given square is attacked by the given side, false otherwise.
 */
bool Board::is_square_attacked(const Squares square, const Colors side) const {
  // attacked by white pawns
  if((side == white) && (pawn_attacks[black][square] & state.bitboards[P]))
    return true;

  // attacked by black pawns
  if((side == black) && (pawn_attacks[white][square] & state.bitboards[p]))
    return true;

  // attacked by knights
  if(knight_attacks[square] & ((side == white) ? state.bitboards[N] : state.bitboards[n]))
    return true;

  // attacked by bishops
  if(get_bishop_attacks(square, state.occupancies[both]) & ((side == white) ? state.bitboards[B] : state.bitboards[b]))
    return true;

  // attacked by rooks
  if(get_rook_attacks(square, state.occupancies[both]) & ((side == white) ? state.bitboards[R] : state.bitboards[r]))
    return true;

  // attacked by bishops
  if(get_queen_attacks(square, state.occupancies[both]) & ((side == white) ? state.bitboards[Q] : state.bitboards[q]))
    return true;

  // attacked by kings
  if(king_attacks[square] & ((side == white) ? state.bitboards[K] : state.bitboards[k]))
    return true;

  return false;
}

/**
 * @brief Updates the occupancies of the board state.
 *
 * This function updates the occupancies of the board state by ORing all the bitboards of the pieces of each color together.
 * It first updates the occupancies of the white pieces, then the occupancies of the black pieces, and finally the occupancies of all pieces.
 */
void Board::update_occupancies() {
  state.occupancies[white] = state.bitboards[P] | state.bitboards[N] | state.bitboards[B] | state.bitboards[R] | state.bitboards[Q] | state.bitboards[K];
  state.occupancies[black] = state.bitboards[p] | state.bitboards[n] | state.bitboards[b] | state.bitboards[r] | state.bitboards[q] | state.bitboards[k];
  state.occupancies[both] = state.occupancies[white] | state.occupancies[black];
}

/**
 * @brief Makes a move on the board.
 *
 * This function makes a move on the board. It first checks if the move is of type all_moves, and if so, it makes a copy of the current board state. It then gets the source square, target square, piece, promoted piece, capture, double push, en passant, and castling flags from the move. It then handles the move by moving the piece, handling capture, promotion, en passant, and castling. Finally, it updates the occupancies and en passant, and changes the side of the board state.
 *
 * @param move The move to make.
 * @param move_flag The type of move to make.
 */
bool Board::make_move(const int move, const TypeMove type_move) {
  if(type_move == TypeMove::all_moves) {
    push_state();

    const Squares source_square = Move::get_move_source(move);
    const Squares target_square = Move::get_move_target(move);
    const Pieces piece = Move::get_move_piece(move);
    const Pieces promoted = Move::get_move_promoted(move);
    const bool capture = Move::get_move_capture(move);
    const bool double_push = Move::get_move_double(move);
    const bool en_passant = Move::get_move_enpassant(move);
    const bool castling = Move::get_move_castling(move);

    // move piece
    pop_bit(state.bitboards[piece], source_square);
    set_bit(state.bitboards[piece], target_square);

    // handle capture
    if(capture) {
      int start_piece, end_piece;

      if(state.side == white) {
        start_piece = p;
        end_piece = k;
      } else {
        start_piece = P;
        end_piece = K;
      }

      for(int bb_piece{ start_piece }; bb_piece <= end_piece; ++bb_piece) {
        if(get_bit(state.bitboards[bb_piece], target_square)) {
          pop_bit(state.bitboards[bb_piece], target_square);
          break;
        }
      }
    }

    // handle promotion
    if(promoted != no_pieces) {
      // erase the pawn from the target square
      pop_bit(state.bitboards[(state.side == white) ? P : p], target_square);
      // set up promoted piece on chess board
      set_bit(state.bitboards[promoted], target_square);
    }

    // handle en passant
    if(en_passant) {
      // erase the pawn depending on side to move
      if(state.side == white) {
        pop_bit(state.bitboards[p], target_square + rank_bit);
      } else {
        pop_bit(state.bitboards[P], target_square - rank_bit);
      }
    }

    // reset enpassant square
    state.en_passant = no_square;

    // handle double pawn push
    if(double_push) {
      // set enpassant square depending on side to move
      if(state.side == white) {
        state.en_passant = target_square + rank_bit;
      } else {
        state.en_passant = target_square - rank_bit;
      }
    }

    // handle castling
    if(castling) {
      if(target_square == g1) { // white kingside
        pop_bit(state.bitboards[R], h1);
        set_bit(state.bitboards[R], f1);
      } else if(target_square == c1) { // white queenside
        pop_bit(state.bitboards[R], a1);
        set_bit(state.bitboards[R], d1);
      } else if(target_square == g8) { // black kingside
        pop_bit(state.bitboards[r], h8);
        set_bit(state.bitboards[r], f8);
      } else if(target_square == c8) { // back queenside
        pop_bit(state.bitboards[r], a8);
        set_bit(state.bitboards[r], d8);
      }
    }

    // update castling rights
    state.castle &= castling_rights[source_square];
    state.castle &= castling_rights[target_square];

    // update occupancies
    state.occupancies.fill(zero);

    for(Pieces bb_piece{ P }; bb_piece <= K; ++bb_piece)
      // update white occupancies
      state.occupancies[white] |= state.bitboards[bb_piece];

    // loop over black pieces bitboards
    for(Pieces bb_piece{ p }; bb_piece <= k; ++bb_piece)
      // update black occupancies
      state.occupancies[black] |= state.bitboards[bb_piece];

    // update both sides occupancies
    state.occupancies[both] |= state.occupancies[white];
    state.occupancies[both] |= state.occupancies[black];

    // change side
    state.side = (state.side == white) ? black : white;

    // make sure that king has not been exposed into a check
    if(is_square_attacked((state.side == white) ? get_ls1b_index(state.bitboards[k]) : get_ls1b_index(state.bitboards[K]), state.side)) {
      // take move back
      pop_state();

      // return illegal move
      return false;
    } else {
      return true;
    }
  } else {
    // make sure move is the capture
    if(Move::get_move_capture(move)) {
      return make_move(move, TypeMove::all_moves);

    // otherwise the move is not a capture
     } else {
      // return illegal move
      return false;
    }
  }
}

void Board::generate_moves() {
  moves_list.clear();
  Squares source_square{ no_square }, target_square{ no_square };
  u64 bitboard{}, attacks{};

  for(Pieces piece{ P }; piece < no_pieces; ++piece) {
    bitboard = state.bitboards[piece];

    if(state.side == white) {
      if(piece == P) {
        while(bitboard) {
          source_square = get_ls1b_index(bitboard);
          target_square = source_square - rank_bit;

          if((target_square != no_square) && !get_bit(state.occupancies[both], target_square)) {
            if(source_square >= a7 && source_square <= h7) {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, Q, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, R, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, B, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, N, false, false, false, false)));
            } else {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
              if((source_square >= a2 && source_square <= h2) && !get_bit(state.occupancies[both], target_square - rank_bit)) {
                moves_list.add(Move::encode_move(Move(source_square, target_square - rank_bit, piece, no_pieces, false, true, false, false)));
              }
            }
          }

          attacks = pawn_attacks[state.side][source_square] & state.occupancies[black];

          while(attacks) {
            target_square = get_ls1b_index(attacks);

            if(source_square >= a7 && source_square <= h7) {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, Q, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, R, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, B, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, N, true, false, false, false)));
            } else {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));
            }

            pop_bit(attacks, target_square);
          }

          if(state.en_passant != no_square) {
            if(const u64 en_passant_attacks = pawn_attacks[state.side][source_square] & (one << state.en_passant)) {
              const auto target_en_passant = get_ls1b_index(en_passant_attacks);
              moves_list.add(Move::encode_move(Move(source_square, target_en_passant, piece, no_pieces, true, false, true, false)));
            }
          }

          pop_bit(bitboard, source_square);
        }
      }

      if(piece == K) {
        if((state.castle & wk) && !get_bit(state.occupancies[both], f1) && !get_bit(state.occupancies[both], g1) && !is_square_attacked(e1, black) &&
           !is_square_attacked(f1, black) && !is_square_attacked(g1, black))
          moves_list.add(Move::encode_move(Move(e1, g1, piece, no_pieces, false, false, false, true)));

        if((state.castle & wq) && !get_bit(state.occupancies[both], d1) && !get_bit(state.occupancies[both], c1) && !get_bit(state.occupancies[both], b1) &&
           !is_square_attacked(e1, black) && !is_square_attacked(d1, black) && !is_square_attacked(c1, black))
          moves_list.add(Move::encode_move(Move(e1, c1, piece, no_pieces, false, false, false, true)));
      }
    }

    if(state.side == black) {
      if(piece == p) {
        while(bitboard) {
          source_square = get_ls1b_index(bitboard);
          target_square = source_square + rank_bit;

          if((target_square != no_square) && !get_bit(state.occupancies[both], target_square)) {
            if(source_square >= a2 && source_square <= h2) {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, q, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, r, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, b, false, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, n, false, false, false, false)));
            } else {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
              if((source_square >= a7 && source_square <= h7) && !get_bit(state.occupancies[both], target_square + rank_bit)) {
                moves_list.add(Move::encode_move(Move(source_square, target_square + rank_bit, piece, no_pieces, false, true, false, false)));
              }
            }
          }

          attacks = pawn_attacks[state.side][source_square] & state.occupancies[white];

          while(attacks) {
            target_square = get_ls1b_index(attacks);

            if(source_square >= a2 && source_square <= h2) {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, q, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, r, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, b, true, false, false, false)));
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, n, true, false, false, false)));
            } else {
              moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));
            }

            pop_bit(attacks, target_square);
          }

          if(state.en_passant != no_square) {
            if(const u64 enpassant_attacks = pawn_attacks[state.side][source_square] & (one << state.en_passant)) {
              const Squares target_enpassant = get_ls1b_index(enpassant_attacks);
              moves_list.add(Move::encode_move(Move(source_square, target_enpassant, piece, no_pieces, true, false, true, false)));
            }
          }

          pop_bit(bitboard, source_square);
        }
      }

      if(piece == k) {
        if((state.castle & bk) && !get_bit(state.occupancies[both], f8) && !get_bit(state.occupancies[both], g8) && !is_square_attacked(e8, white) &&
           !is_square_attacked(f8, white) && !is_square_attacked(g8, white))
          moves_list.add(Move::encode_move(Move(e8, g8, piece, no_pieces, false, false, false, true)));

        if((state.castle & bq) && !get_bit(state.occupancies[both], d8) && !get_bit(state.occupancies[both], c8) && !get_bit(state.occupancies[both], b8) &&
           !is_square_attacked(e8, white) && !is_square_attacked(d8, white) && !is_square_attacked(c8, white))
          moves_list.add(Move::encode_move(Move(e8, c8, piece, no_pieces, false, false, false, true)));
      }
    }

    if(const Pieces targetKnight = (state.side == white) ? N : n; piece == targetKnight) {
      while(bitboard) {
        source_square = get_ls1b_index(bitboard);
        attacks = knight_attacks[source_square] & ((state.side == white) ? ~state.occupancies[white] : ~state.occupancies[black]);

        while(attacks) {
          target_square = get_ls1b_index(attacks);

          if(!get_bit(((state.side == white) ? state.occupancies[black] : state.occupancies[white]), target_square))
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
          else
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));

          pop_bit(attacks, target_square);
        }

        pop_bit(bitboard, source_square);
      }
    }

    if(const Pieces targetBishop = (state.side == white) ? B : b; piece == targetBishop) {
      while(bitboard) {
        source_square = get_ls1b_index(bitboard);

        attacks = get_bishop_attacks(source_square, state.occupancies[both]) & (state.side == white ? ~state.occupancies[white] : ~state.occupancies[black]);

        while(attacks) {
          target_square = get_ls1b_index(attacks);

          if(!get_bit(((state.side == white) ? state.occupancies[black] : state.occupancies[white]), target_square))
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
          else
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));
          pop_bit(attacks, target_square);
        }

        pop_bit(bitboard, source_square);
      }
    }

    if(const Pieces targetRook = (state.side == white) ? R : r; piece == targetRook) {
      while(bitboard) {
        source_square = get_ls1b_index(bitboard);
        attacks = get_rook_attacks(source_square, state.occupancies[both]) & ((state.side == white) ? ~state.occupancies[white] : ~state.occupancies[black]);

        while(attacks) {
          target_square = get_ls1b_index(attacks);

          if(!get_bit(((state.side == white) ? state.occupancies[black] : state.occupancies[white]), target_square))
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
          else
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));

          pop_bit(attacks, target_square);
        }

        pop_bit(bitboard, source_square);
      }
    }

    if(const Pieces targetQueen = (state.side == white) ? Q : q; piece == targetQueen) {
      while(bitboard) {
        source_square = get_ls1b_index(bitboard);

        attacks = get_queen_attacks(source_square, state.occupancies[both]) & ((state.side == white) ? ~state.occupancies[white] : ~state.occupancies[black]);

        while(attacks) {
          target_square = get_ls1b_index(attacks);

          if(!get_bit(((state.side == white) ? state.occupancies[black] : state.occupancies[white]), target_square))
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
          else
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));

          pop_bit(attacks, target_square);
        }

        pop_bit(bitboard, source_square);
      }
    }

    if(const Pieces targetKing = (state.side == white) ? K : k; piece == targetKing) {
      while(bitboard) {
        source_square = get_ls1b_index(bitboard);

        attacks = king_attacks[source_square] & ((state.side == white) ? ~state.occupancies[white] : ~state.occupancies[black]);

        while(attacks) {
          target_square = get_ls1b_index(attacks);

          if(!get_bit(((state.side == white) ? state.occupancies[black] : state.occupancies[white]), target_square))
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, false, false, false, false)));
          else
            moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, true, false, false, false)));

          pop_bit(attacks, target_square);
        }

        pop_bit(bitboard, source_square);
      }
    }
  }
}

} // namespace maharajah
