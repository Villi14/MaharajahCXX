#include "../headers/Board.h"
#include "../headers/Bitboard.h"
#include "../headers/Notation.h"
#include "../headers/Zobrist.h"

#include <charconv>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace maharajah {

namespace {

// white and black counterparts of the pieces that can be generated generically
constexpr array<Pieces, 7> white_officers{ N, B, R, Q, A, C, M };
constexpr array<Pieces, 7> black_officers{ n, b, r, q, a, c, m };

constexpr array<Pieces, 4> white_promotions{ Q, R, B, N };
constexpr array<Pieces, 4> black_promotions{ q, r, b, n };
constexpr array<Pieces, 3> white_compound_promotions{ A, C, M };
constexpr array<Pieces, 3> black_compound_promotions{ a, c, m };

void add_promotions(MoveList& moves_list, const BoardState& state, const Squares source, const Squares target, const Pieces pawn, const bool capture) {
  const bool is_white = state.side == white;

  for(const Pieces promoted : is_white ? white_promotions : black_promotions)
    moves_list.add(Move::encode_move(Move(source, target, pawn, promoted, capture, false, false, false)));

  // compound promotions exist only for a side playing variant rules
  if(state.side_variant[state.side]) {
    for(const Pieces promoted : is_white ? white_compound_promotions : black_compound_promotions)
      moves_list.add(Move::encode_move(Move(source, target, pawn, promoted, capture, false, false, false)));
  }
}

Squares parse_square(const string_view text) {
  if(text.size() != 2 || text[0] < 'a' || text[0] > 'h' || text[1] < '1' || text[1] > '8')
    return no_square;
  return to_square((BoardGeometry::ranks - (text[1] - '0')) * BoardGeometry::ranks + (text[0] - 'a'));
}

} // namespace

int Board::repetition_count() const {
  int count = 1;

  for(int index{ }; index < repetition_index; ++index) {
    if(repetition_table[index] == state.hash_key)
      ++count;
  }

  return count;
}

bool Board::is_square_attacked(const Squares square, const Colors side) const {
  const auto& bb = state.bitboards;
  const u64 occupancy = state.occupancies[both];
  const bool is_white = side == white;

  // attacked by pawns
  if(attack_tables.pawn[opponent(side)][square] & bb[is_white ? P : p])
    return true;

  // attacked by knights
  if(attack_tables.knight[square] & bb[is_white ? N : n])
    return true;

  // attacked by bishops and archbishops
  if(get_bishop_attacks(square, occupancy) & bb[is_white ? B : b])
    return true;

  if(get_archbishop_attacks(square, occupancy) & bb[is_white ? A : a])
    return true;

  // attacked by rooks and chancellors
  if(get_rook_attacks(square, occupancy) & bb[is_white ? R : r])
    return true;

  if(get_chancellor_attacks(square, occupancy) & bb[is_white ? C : c])
    return true;

  // attacked by queens and amazons
  if(get_queen_attacks(square, occupancy) & bb[is_white ? Q : q])
    return true;

  if(get_amazon_attacks(square, occupancy) & bb[is_white ? M : m])
    return true;

  // attacked by kings
  if(attack_tables.king[square] & bb[is_white ? K : k])
    return true;

  return false;
}

bool Board::in_check() const {
  const u64 king = state.bitboards[state.side == white ? K : k];
  return king && is_square_attacked(get_ls1b_index(king), opponent(state.side));
}

void Board::update_occupancies() {
  state.occupancies.fill(zero);

  for(Pieces piece{ P }; piece <= K; ++piece)
    state.occupancies[white] |= state.bitboards[piece];

  for(Pieces piece{ p }; piece <= k; ++piece)
    state.occupancies[black] |= state.bitboards[piece];

  state.occupancies[both] = state.occupancies[white] | state.occupancies[black];
}

bool Board::make_move(const int move, const TypeMove type_move) {
  if(type_move == TypeMove::only_captures && !Move::get_move_capture(move))
    return false;

  push_state();

  const ZobristKeys& keys = ZobristKeys::get();
  const Squares source_square = Move::get_move_source(move);
  const Squares target_square = Move::get_move_target(move);
  const Pieces piece = Move::get_move_piece(move);
  const Pieces promoted = Move::get_move_promoted(move);
  const bool capture = Move::get_move_capture(move);
  const bool double_push = Move::get_move_double(move);
  const bool en_passant = Move::get_move_enpassant(move);
  const bool castling = Move::get_move_castling(move);
  const Colors side = state.side;
  const Colors enemy = opponent(side);

  // halfmove clock: reset on pawn move or capture
  if(piece == P || piece == p || capture || en_passant)
    state.halfmove = 0;
  else
    ++state.halfmove;

  // move piece
  pop_bit(state.bitboards[piece], source_square);
  set_bit(state.bitboards[piece], target_square);
  pop_bit(state.occupancies[side], source_square);
  set_bit(state.occupancies[side], target_square);
  state.hash_key ^= keys.piece[piece][source_square];
  state.hash_key ^= keys.piece[piece][target_square];

  // the mover leaves its square, and an unmoved pawn on the target square is captured
  pop_bit(state.pawn_unmoved, source_square);
  pop_bit(state.pawn_unmoved, target_square);

  // handle capture; capturing a king is never legal
  if(capture) {
    const Pieces start_piece = (side == white) ? p : P;
    const Pieces end_piece = (side == white) ? k : K;

    for(Pieces bb_piece{ start_piece }; bb_piece <= end_piece; ++bb_piece) {
      if(get_bit(state.bitboards[bb_piece], target_square)) {
        if(bb_piece == K || bb_piece == k) {
          pop_state();
          return false;
        }

        pop_bit(state.bitboards[bb_piece], target_square);
        pop_bit(state.occupancies[enemy], target_square);
        state.hash_key ^= keys.piece[bb_piece][target_square];
        break;
      }
    }
  }

  // handle promotion
  if(promoted != no_pieces) {
    const Pieces pawn = (side == white) ? P : p;
    pop_bit(state.bitboards[pawn], target_square);
    state.hash_key ^= keys.piece[pawn][target_square];
    set_bit(state.bitboards[promoted], target_square);
    state.hash_key ^= keys.piece[promoted][target_square];
  }

  // handle en passant
  if(en_passant) {
    const Squares captured_square = (side == white) ? target_square + BoardGeometry::ranks : target_square - BoardGeometry::ranks;
    const Pieces captured_pawn = (side == white) ? p : P;
    pop_bit(state.bitboards[captured_pawn], captured_square);
    pop_bit(state.occupancies[enemy], captured_square);
    pop_bit(state.pawn_unmoved, captured_square);
    state.hash_key ^= keys.piece[captured_pawn][captured_square];
  }

  // reset enpassant square
  if(state.en_passant != no_square)
    state.hash_key ^= keys.en_passant[state.en_passant];
  state.en_passant = no_square;

  // handle double pawn push
  if(double_push) {
    state.en_passant = (side == white) ? target_square + BoardGeometry::ranks : target_square - BoardGeometry::ranks;
    state.hash_key ^= keys.en_passant[state.en_passant];
  }

  // handle castling
  if(castling) {
    const auto move_rook = [&](const Pieces rook, const Squares from, const Squares to) {
      pop_bit(state.bitboards[rook], from);
      set_bit(state.bitboards[rook], to);
      pop_bit(state.occupancies[side], from);
      set_bit(state.occupancies[side], to);
      state.hash_key ^= keys.piece[rook][from];
      state.hash_key ^= keys.piece[rook][to];
    };

    if(target_square == g1) // white kingside
      move_rook(R, h1, f1);
    else if(target_square == c1) // white queenside
      move_rook(R, a1, d1);
    else if(target_square == g8) // black kingside
      move_rook(r, h8, f8);
    else if(target_square == c8) // black queenside
      move_rook(r, a8, d8);
  }

  // update castling rights
  state.hash_key ^= keys.castle[state.castle];
  state.castle &= CastlingRules::rights[source_square];
  state.castle &= CastlingRules::rights[target_square];
  state.hash_key ^= keys.castle[state.castle];

  state.occupancies[both] = state.occupancies[white] | state.occupancies[black];

  // change side
  state.side = enemy;
  state.hash_key ^= keys.side;

  // make sure that the mover's king has not been exposed into a check
  // (a position without that king cannot be in check)
  const u64 mover_king = state.bitboards[side == white ? K : k];
  if(mover_king && is_square_attacked(get_ls1b_index(mover_king), enemy)) {
    pop_state();
    return false;
  }

  return true;
}

bool Board::has_legal_move() {
  MoveList list;
  generate_moves(list);

  for(size_t i{ }; i < list.size(); ++i) {
    if(make_move(list[i], TypeMove::all_moves)) {
      pop_state();
      return true;
    }
  }

  return false;
}

void Board::generate_moves() {
  generate_moves(moves_list);
}

void Board::generate_moves(MoveList& moves_list) const {
  moves_list.clear();

  const Colors side = state.side;
  const bool is_white = side == white;
  const u64 occupancy = state.occupancies[both];
  const u64 own = state.occupancies[side];
  const u64 enemy = state.occupancies[opponent(side)];

  // pawns
  {
    const Pieces pawn = is_white ? P : p;
    const int forward = is_white ? -BoardGeometry::ranks : BoardGeometry::ranks;
    u64 bitboard = state.bitboards[pawn];

    while(bitboard) {
      const Squares source_square = get_ls1b_index(bitboard);
      const Squares target_square = source_square + forward;
      const bool promotes = is_white ? (source_square >= a7 && source_square <= h7) : (source_square >= a2 && source_square <= h2);

      if(target_square != no_square && !get_bit(occupancy, target_square)) {
        if(promotes) {
          add_promotions(moves_list, state, source_square, target_square, pawn, false);
        } else {
          moves_list.add(Move::encode_move(Move(source_square, target_square, pawn, no_pieces, false, false, false, false)));

          // Double step: from the home rank under standard rules, from any rank under
          // variant rules (custom armies start anywhere), or exactly the unmoved pawns
          // when the position carries per-pawn state.
          const Squares double_target = target_square + forward;
          bool can_double;
          if(state.has_pawn_state) {
            can_double = get_bit(state.pawn_unmoved, source_square);
          } else {
            const bool on_home_rank = is_white ? (source_square >= a2 && source_square <= h2) : (source_square >= a7 && source_square <= h7);
            can_double = on_home_rank || (state.side_variant[side] && double_target != no_square);
          }

          if(can_double && double_target != no_square && !get_bit(occupancy, double_target))
            moves_list.add(Move::encode_move(Move(source_square, double_target, pawn, no_pieces, false, true, false, false)));
        }
      }

      u64 attacks = attack_tables.pawn[side][source_square] & enemy;

      while(attacks) {
        const Squares capture_square = get_ls1b_index(attacks);

        if(promotes)
          add_promotions(moves_list, state, source_square, capture_square, pawn, true);
        else
          moves_list.add(Move::encode_move(Move(source_square, capture_square, pawn, no_pieces, true, false, false, false)));

        pop_bit(attacks, capture_square);
      }

      if(state.en_passant != no_square) {
        if(const u64 en_passant_attacks = attack_tables.pawn[side][source_square] & (one << state.en_passant)) {
          const Squares en_passant_target = get_ls1b_index(en_passant_attacks);
          moves_list.add(Move::encode_move(Move(source_square, en_passant_target, pawn, no_pieces, true, false, true, false)));
        }
      }

      pop_bit(bitboard, source_square);
    }
  }

  const auto add_piece_moves = [&](const Pieces piece) {
    u64 bitboard = state.bitboards[piece];

    while(bitboard) {
      const Squares source_square = get_ls1b_index(bitboard);
      u64 attacks = get_piece_attacks(piece, source_square, occupancy) & ~own;

      while(attacks) {
        const Squares target_square = get_ls1b_index(attacks);
        const bool capture = get_bit(enemy, target_square);
        moves_list.add(Move::encode_move(Move(source_square, target_square, piece, no_pieces, capture, false, false, false)));
        pop_bit(attacks, target_square);
      }

      pop_bit(bitboard, source_square);
    }
  };

  for(const Pieces piece : is_white ? white_officers : black_officers)
    add_piece_moves(piece);

  // castling is not available to a side playing variant rules
  if(!state.side_variant[side]) {
    if(is_white) {
      if((state.castle & wk) && !get_bit(occupancy, f1) && !get_bit(occupancy, g1) && !is_square_attacked(e1, black) && !is_square_attacked(f1, black) &&
         !is_square_attacked(g1, black))
        moves_list.add(Move::encode_move(Move(e1, g1, K, no_pieces, false, false, false, true)));

      if((state.castle & wq) && !get_bit(occupancy, d1) && !get_bit(occupancy, c1) && !get_bit(occupancy, b1) && !is_square_attacked(e1, black) &&
         !is_square_attacked(d1, black) && !is_square_attacked(c1, black))
        moves_list.add(Move::encode_move(Move(e1, c1, K, no_pieces, false, false, false, true)));
    } else {
      if((state.castle & bk) && !get_bit(occupancy, f8) && !get_bit(occupancy, g8) && !is_square_attacked(e8, white) && !is_square_attacked(f8, white) &&
         !is_square_attacked(g8, white))
        moves_list.add(Move::encode_move(Move(e8, g8, k, no_pieces, false, false, false, true)));

      if((state.castle & bq) && !get_bit(occupancy, d8) && !get_bit(occupancy, c8) && !get_bit(occupancy, b8) && !is_square_attacked(e8, white) &&
         !is_square_attacked(d8, white) && !is_square_attacked(c8, white))
        moves_list.add(Move::encode_move(Move(e8, c8, k, no_pieces, false, false, false, true)));
    }
  }

  add_piece_moves(is_white ? K : k);
}

void Board::parse_fen(const string_view fen) {
  const auto invalid = [] { return runtime_error("Invalid FEN"); };

  string placement, side_field, castle_field, en_passant_field, halfmove_field, fullmove_field, variant_field, pawn_field;
  istringstream fields{ string(fen) };
  if(!(fields >> placement >> side_field >> castle_field >> en_passant_field))
    throw invalid();
  // optional: halfmove clock, fullmove number, variant rights, unmoved pawns
  fields >> halfmove_field >> fullmove_field >> variant_field >> pawn_field;

  // parse into a temporary so a malformed FEN leaves the board untouched
  BoardState parsed{ };

  int rank{ }, file{ };
  for(const char ch : placement) {
    if(ch == '/') {
      if(file != BoardGeometry::files || ++rank >= BoardGeometry::ranks)
        throw invalid();
      file = 0;
    } else if(ch >= '1' && ch <= '8') {
      file += ch - '0';
      if(file > BoardGeometry::files)
        throw invalid();
    } else {
      const auto piece = Fen::piece_chars.find(ch);
      if(piece == string_view::npos || file >= BoardGeometry::files)
        throw invalid();
      set_bit(parsed.bitboards[piece], to_square(rank * BoardGeometry::ranks + file));
      ++file;
    }
  }
  if(rank != BoardGeometry::ranks - 1 || file != BoardGeometry::files)
    throw invalid();

  if(side_field == "w")
    parsed.side = white;
  else if(side_field == "b")
    parsed.side = black;
  else
    throw invalid();

  if(castle_field != "-") {
    for(const char ch : castle_field) {
      switch(ch) {
      case 'K':
        parsed.castle |= wk;
        break;
      case 'Q':
        parsed.castle |= wq;
        break;
      case 'k':
        parsed.castle |= bk;
        break;
      case 'q':
        parsed.castle |= bq;
        break;
      default:
        throw invalid();
      }
    }
  }

  if(en_passant_field != "-") {
    parsed.en_passant = parse_square(en_passant_field);
    if(parsed.en_passant == no_square)
      throw invalid();
  }

  if(!halfmove_field.empty())
    from_chars(halfmove_field.data(), halfmove_field.data() + halfmove_field.size(), parsed.halfmove);

  if(!pawn_field.empty()) {
    parsed.has_pawn_state = true;
    if(pawn_field != "-") {
      if(pawn_field.size() % 2 != 0)
        throw invalid();
      for(size_t i{ }; i < pawn_field.size(); i += 2) {
        const Squares square = parse_square(string_view(pawn_field).substr(i, 2));
        if(square == no_square)
          throw invalid();
        set_bit(parsed.pawn_unmoved, square);
      }
    }
  }

  parsed.standard_rules = !parsed.has_compound_pieces();

  if(!variant_field.empty()) {
    for(const char ch : variant_field) {
      if(ch == 'V')
        parsed.side_variant[white] = true;
      else if(ch == 'v')
        parsed.side_variant[black] = true;
      else if(ch != '-')
        throw invalid();
    }
  } else {
    // no per-side field: a board carrying compound material plays variant rules on both sides
    parsed.side_variant = { !parsed.standard_rules, !parsed.standard_rules };
  }

  // a variant side never castles, whatever the castling field claims
  if(parsed.side_variant[white])
    parsed.castle &= ~(wk | wq);
  if(parsed.side_variant[black])
    parsed.castle &= ~(bk | bq);

  state = parsed;
  update_occupancies();
  state.hash_key = generate_hash_key(state);

  ply = 0;
  repetition_index = 0;
  repetition_table.assign(1, zero);
}

string Board::to_fen(int fullmove_number) const {
  string fen;

  for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
    int empty{ };
    for(int file{ }; file < BoardGeometry::files; ++file) {
      const Squares square = to_square(rank * BoardGeometry::ranks + file);
      Pieces found{ no_pieces };
      for(Pieces piece{ P }; piece <= k; ++piece) {
        if(get_bit(state.bitboards[piece], square)) {
          found = piece;
          break;
        }
      }

      if(found == no_pieces) {
        ++empty;
        continue;
      }

      if(empty > 0) {
        fen += to_string(empty);
        empty = 0;
      }
      fen += Fen::piece_chars[found];
    }

    if(empty > 0)
      fen += to_string(empty);
    if(rank < BoardGeometry::ranks - 1)
      fen += '/';
  }

  fen += state.side == white ? " w " : " b ";

  if(state.castle == 0) {
    fen += '-';
  } else {
    if(state.castle & wk)
      fen += 'K';
    if(state.castle & wq)
      fen += 'Q';
    if(state.castle & bk)
      fen += 'k';
    if(state.castle & bq)
      fen += 'q';
  }

  fen += ' ';
  fen += (state.en_passant != no_square) ? Notation::square_to_coordinates[state.en_passant] : "-";

  if(fullmove_number < 1)
    fullmove_number = 1;
  fen += ' ' + to_string(state.halfmove) + ' ' + to_string(fullmove_number) + ' ';

  if(state.side_variant[white])
    fen += 'V';
  if(state.side_variant[black])
    fen += 'v';
  if(!state.side_variant[white] && !state.side_variant[black])
    fen += '-';

  if(state.has_pawn_state) {
    // squares whose pawn is gone are dropped
    u64 unmoved_pawns = state.pawn_unmoved & (state.bitboards[P] | state.bitboards[p]);
    fen += ' ';
    if(!unmoved_pawns)
      fen += '-';
    while(unmoved_pawns) {
      const Squares square = get_ls1b_index(unmoved_pawns);
      fen += Notation::square_to_coordinates[square];
      pop_bit(unmoved_pawns, square);
    }
  }

  return fen;
}

int Board::parse_move(string_view move_string) const {
  move_string = move_string.substr(0, move_string.find_first_of(" \t\r\n"));
  if(move_string.size() != 4 && move_string.size() != 5)
    return 0;

  const Squares source_square = parse_square(move_string.substr(0, 2));
  const Squares target_square = parse_square(move_string.substr(2, 2));
  if(source_square == no_square || target_square == no_square)
    return 0;

  MoveList move_list;
  generate_moves(move_list);

  for(size_t i{ }; i < move_list.size(); ++i) {
    const int move = move_list[i];

    if(source_square != Move::get_move_source(move) || target_square != Move::get_move_target(move))
      continue;

    if(Move::is_promotion(move)) {
      if(move_string.size() == 5 && Notation::promoted_pieces[Move::get_move_promoted(move)] == move_string[4])
        return move;
      // continue the loop on other promotions (e.g. "e7e8r" when looking at e7e8q)
      continue;
    }

    if(move_string.size() == 4)
      return move;
  }

  // illegal move
  return 0;
}

string Board::move_to_string(const int move) {
  string text = string(Notation::square_to_coordinates[Move::get_move_source(move)]) + Notation::square_to_coordinates[Move::get_move_target(move)];
  if(Move::is_promotion(move))
    text += Notation::promoted_pieces[Move::get_move_promoted(move)];
  return text;
}

} // namespace maharajah
