#include "../headers/Board.h"
#include "../headers/Bitboard.h"
#include "../headers/Move.h"
#include "../headers/Notation.h"
#include "../headers/Zobrist.h"

#include <algorithm>
#include <charconv>
#include <initializer_list>
#include <sstream>

using namespace std;

namespace maharajah {

namespace {

// the pieces other than pawns whose moves are generated generically, white kinds,
// in generation order (the king comes last, after castling)
constexpr array<Pieces, 7> officers{ N, B, R, Q, A, C, M };

// white kinds, in generation order
constexpr array<Pieces, 4> promotions{ Q, R, B, N };
constexpr array<Pieces, 3> compound_promotions{ A, C, M };

constexpr u64 squares_mask(const initializer_list<Squares> squares) {
  u64 mask{ };
  for(const Squares square : squares)
    mask |= one << square;
  return mask;
}

// One castling move: the right it needs, the king's and the rook's moves, the squares
// that must be empty and the squares the king starts on, passes and lands on, none of
// which may be attacked.
struct CastlingMove {
  Castling right;
  Colors side;
  Squares king_from;
  Squares king_to;
  Squares rook_from;
  Squares rook_to;
  u64 empty;
  array<Squares, 3> king_path;
};

// white kingside, white queenside, black kingside, black queenside: the order the
// moves are generated in
constexpr array<CastlingMove, 4> castling_moves{ {
    { wk, white, e1, g1, h1, f1, squares_mask({ f1, g1 }), { e1, f1, g1 } },
    { wq, white, e1, c1, a1, d1, squares_mask({ d1, c1, b1 }), { e1, d1, c1 } },
    { bk, black, e8, g8, h8, f8, squares_mask({ f8, g8 }), { e8, f8, g8 } },
    { bq, black, e8, c8, a8, d8, squares_mask({ d8, c8, b8 }), { e8, d8, c8 } },
} };

void add_promotions(MoveList& moves_list, const BoardState& state, const Squares source, const Squares target, const Pieces pawn, const bool capture) {
  for(const Pieces promoted : promotions)
    moves_list.add(Move::promotion_move(source, target, pawn, colored(promoted, state.side), capture));

  // compound promotions exist only for a side playing variant rules
  if(state.side_variant[state.side]) {
    for(const Pieces promoted : compound_promotions)
      moves_list.add(Move::promotion_move(source, target, pawn, colored(promoted, state.side), capture));
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

bool Board::is_draw() const {
  // threefold and fivefold repetition, fifty and seventy-five move rules
  return repetition_count() >= 3 || (state.standard_rules && state.halfmove >= 100) || is_insufficient_material();
}

bool Board::is_insufficient_material() const {
  if(!state.standard_rules)
    return false;

  const auto count = [&](const Pieces piece) { return count_bits(state.bitboards[piece]); };

  for(const Pieces piece : { P, p, R, r, Q, q, A, a, C, c, M, m }) {
    if(count(piece))
      return false;
  }

  const int white_knights = count(N);
  const int black_knights = count(n);
  const int white_bishops = count(B);
  const int black_bishops = count(b);
  const int white_minors = white_knights + white_bishops;
  const int black_minors = black_knights + black_bishops;

  // Dead positions only: K vs K, K + single minor vs K, and KB vs KB with both
  // bishops on the same square colour. KNN vs K, KN vs KN and KB vs KN are not dead.
  if(white_minors == 0 && black_minors == 0)
    return true;

  if(white_minors + black_minors == 1)
    return true;

  if(white_knights == 0 && black_knights == 0 && white_bishops == 1 && black_bishops == 1) {
    const int white_square = get_ls1b_index(state.bitboards[B]);
    const int black_square = get_ls1b_index(state.bitboards[b]);
    // square colour is the parity of rank + file
    return (((white_square >> 3) ^ white_square) & 1) == (((black_square >> 3) ^ black_square) & 1);
  }

  return false;
}

bool Board::is_square_attacked(const Squares square, const Colors side) const {
  const u64 occupancy = state.occupancies[both];
  const AttackerGroups attackers(state.bitboards, side);

  // a pawn of `side` attacks the square if a pawn of the other colour on the square
  // would attack it back
  return (attack_tables.pawn[opponent(side)][square] & attackers.pawns) || (attack_tables.knight[square] & attackers.knight_movers) ||
         (get_bishop_attacks(square, occupancy) & attackers.diagonal_movers) || (get_rook_attacks(square, occupancy) & attackers.line_movers) ||
         (attack_tables.king[square] & attackers.king);
}

bool Board::in_check() const {
  const u64 king = state.bitboards[colored(K, state.side)];
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

  // handle capture (en passant takes a pawn off another square, below); capturing a
  // king is never legal
  if(capture) {
    const Pieces victim = state.piece_on(target_square, enemy);
    if(victim == colored(K, enemy)) {
      pop_state();
      return false;
    }

    if(victim != no_pieces) {
      pop_bit(state.bitboards[victim], target_square);
      pop_bit(state.occupancies[enemy], target_square);
      state.hash_key ^= keys.piece[victim][target_square];
    }
  }

  // handle promotion
  if(promoted != no_pieces) {
    const Pieces pawn = colored(P, side);
    pop_bit(state.bitboards[pawn], target_square);
    state.hash_key ^= keys.piece[pawn][target_square];
    set_bit(state.bitboards[promoted], target_square);
    state.hash_key ^= keys.piece[promoted][target_square];
  }

  // handle en passant
  if(en_passant) {
    const Squares captured_square = square_behind(target_square, side);
    const Pieces captured_pawn = colored(P, enemy);
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
    state.en_passant = square_behind(target_square, side);
    state.hash_key ^= keys.en_passant[state.en_passant];
  }

  // handle castling: the king has moved, now the rook
  if(castling) {
    for(const CastlingMove& castling_move : castling_moves) {
      if(castling_move.king_to != target_square)
        continue;

      const Pieces rook = colored(R, side);
      pop_bit(state.bitboards[rook], castling_move.rook_from);
      set_bit(state.bitboards[rook], castling_move.rook_to);
      pop_bit(state.occupancies[side], castling_move.rook_from);
      set_bit(state.occupancies[side], castling_move.rook_to);
      state.hash_key ^= keys.piece[rook][castling_move.rook_from];
      state.hash_key ^= keys.piece[rook][castling_move.rook_to];
      break;
    }
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
  const u64 mover_king = state.bitboards[colored(K, side)];
  if(mover_king && is_square_attacked(get_ls1b_index(mover_king), enemy)) {
    pop_state();
    return false;
  }

  return true;
}

bool Board::has_legal_move() {
  MoveList list;
  generate_moves(list);

  for(const int move : list) {
    if(make_move(move, TypeMove::all_moves)) {
      pop_state();
      return true;
    }
  }

  return false;
}

void Board::make_null_move() {
  push_state();

  const ZobristKeys& keys = ZobristKeys::get();
  if(state.en_passant != no_square)
    state.hash_key ^= keys.en_passant[state.en_passant];
  state.en_passant = no_square;
  state.side = opponent(state.side);
  state.hash_key ^= keys.side;
}

void Board::generate_moves(MoveList& out_moves, const TypeMove type_move) const {
  out_moves.clear();

  const bool captures_only = type_move == TypeMove::only_captures;
  const Colors side = state.side;
  const bool is_white = side == white;
  const u64 occupancy = state.occupancies[both];
  const u64 own = state.occupancies[side];
  const u64 enemy = state.occupancies[opponent(side)];

  // pawns
  {
    const Pieces pawn = colored(P, side);
    const int forward = is_white ? -BoardGeometry::ranks : BoardGeometry::ranks;
    // rows count from the 8th rank (row 0): a pawn on row 1 (white) or 6 (black)
    // promotes with its next step, one on row 6 (white) or 1 (black) stands on its
    // home rank
    const int promotion_row = is_white ? 1 : 6;
    const int home_row = is_white ? 6 : 1;
    const auto row_of = [](const Squares square) { return square / BoardGeometry::files; };
    u64 bitboard = state.bitboards[pawn];

    while(bitboard) {
      const Squares source_square = pop_ls1b(bitboard);
      const Squares target_square = source_square + forward;
      const bool promotes = row_of(source_square) == promotion_row;

      if(!captures_only && target_square != no_square && !get_bit(occupancy, target_square)) {
        if(promotes) {
          add_promotions(out_moves, state, source_square, target_square, pawn, false);
        } else {
          out_moves.add(Move::quiet_move(source_square, target_square, pawn));

          // Double step, once per pawn: a variant side's pawn only while field 8 lists
          // it (its army may start on any rank of its half), a classic side's from the
          // home rank.
          const Squares double_target = target_square + forward;
          const bool can_double = state.side_variant[side] ? get_bit(state.pawn_unmoved, source_square) : row_of(source_square) == home_row;

          if(can_double && double_target != no_square && !get_bit(occupancy, double_target))
            out_moves.add(Move::double_push_move(source_square, double_target, pawn));
        }
      }

      u64 attacks = attack_tables.pawn[side][source_square] & enemy;

      while(attacks) {
        const Squares capture_square = pop_ls1b(attacks);

        if(promotes)
          add_promotions(out_moves, state, source_square, capture_square, pawn, true);
        else
          out_moves.add(Move::capture_move(source_square, capture_square, pawn));
      }

      if(state.en_passant != no_square) {
        if(const u64 en_passant_attacks = attack_tables.pawn[side][source_square] & (one << state.en_passant))
          out_moves.add(Move::en_passant_move(source_square, get_ls1b_index(en_passant_attacks), pawn));
      }
    }
  }

  const auto add_piece_moves = [&](const Pieces piece) {
    u64 bitboard = state.bitboards[piece];

    while(bitboard) {
      const Squares source_square = pop_ls1b(bitboard);
      u64 attacks = get_piece_attacks(piece, source_square, occupancy) & (captures_only ? enemy : ~own);

      while(attacks) {
        const Squares target_square = pop_ls1b(attacks);
        out_moves.add(get_bit(enemy, target_square) ? Move::capture_move(source_square, target_square, piece)
                                                    : Move::quiet_move(source_square, target_square, piece));
      }
    }
  };

  for(const Pieces piece : officers)
    add_piece_moves(colored(piece, side));

  // castling is not available to a side playing variant rules
  if(!captures_only && !state.side_variant[side]) {
    for(const CastlingMove& castling_move : castling_moves) {
      if(castling_move.side != side || !(state.castle & castling_move.right) || (occupancy & castling_move.empty))
        continue;

      const bool path_attacked = ranges::any_of(castling_move.king_path, [&](const Squares square) { return is_square_attacked(square, opponent(side)); });
      if(!path_attacked)
        out_moves.add(Move::castling_move(castling_move.king_from, castling_move.king_to, colored(K, side)));
    }
  }

  add_piece_moves(colored(K, side));
}

expected<void, string_view> Board::parse_fen(const string_view fen) {
  string placement, side_field, castle_field, en_passant_field, halfmove_field, fullmove_field, variant_field, pawn_field;
  istringstream fields{ string(fen) };
  if(!(fields >> placement >> side_field >> castle_field >> en_passant_field))
    return unexpected("invalid fen: missing fields");
  // optional: halfmove clock, fullmove number, variant rights, unmoved pawns
  fields >> halfmove_field >> fullmove_field >> variant_field >> pawn_field;

  // parse into a temporary so a malformed FEN leaves the board untouched
  BoardState parsed{ };

  int rank{ }, file{ };
  for(const char ch : placement) {
    if(ch == '/') {
      if(file != BoardGeometry::files || ++rank >= BoardGeometry::ranks)
        return unexpected("invalid fen: piece placement");
      file = 0;
    } else if(ch >= '1' && ch <= '8') {
      file += ch - '0';
      if(file > BoardGeometry::files)
        return unexpected("invalid fen: piece placement");
    } else {
      const auto piece = Fen::piece_chars.find(ch);
      if(piece == string_view::npos || file >= BoardGeometry::files)
        return unexpected("invalid fen: piece placement");
      set_bit(parsed.bitboards[piece], to_square(rank * BoardGeometry::ranks + file));
      ++file;
    }
  }
  if(rank != BoardGeometry::ranks - 1 || file != BoardGeometry::files)
    return unexpected("invalid fen: piece placement");

  if(side_field == "w")
    parsed.side = white;
  else if(side_field == "b")
    parsed.side = black;
  else
    return unexpected("invalid fen: side to move");

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
        return unexpected("invalid fen: castling rights");
      }
    }
  }

  if(en_passant_field != "-") {
    parsed.en_passant = parse_square(en_passant_field);
    if(parsed.en_passant == no_square)
      return unexpected("invalid fen: en passant square");
  }

  if(!halfmove_field.empty())
    from_chars(halfmove_field.data(), halfmove_field.data() + halfmove_field.size(), parsed.halfmove);

  if(!pawn_field.empty() && pawn_field != "-") {
    if(pawn_field.size() % 2 != 0)
      return unexpected("invalid fen: unmoved pawns");
    for(size_t i{ }; i < pawn_field.size(); i += 2) {
      const Squares square = parse_square(string_view(pawn_field).substr(i, 2));
      if(square == no_square)
        return unexpected("invalid fen: unmoved pawns");
      set_bit(parsed.pawn_unmoved, square);
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
        return unexpected("invalid fen: variant rules");
    }
  } else {
    // no per-side field: classic by default, variant for a side holding a compound piece
    parsed.side_variant = { parsed.has_compound_pieces(white), parsed.has_compound_pieces(black) };
  }

  // field 8 lists only the variant sides' pawns; without it no variant pawn may double-step
  parsed.pawn_unmoved &= parsed.variant_pawns();

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
  return { };
}

string Board::to_fen(int fullmove_number) const {
  string fen;

  for(int rank{ }; rank < BoardGeometry::ranks; ++rank) {
    int empty{ };
    for(int file{ }; file < BoardGeometry::files; ++file) {
      const Pieces found = state.piece_on(to_square(rank * BoardGeometry::ranks + file));

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

  // field 8 with a variant side only, listing its pawns; squares whose pawn is gone are dropped
  if(state.side_variant[white] || state.side_variant[black]) {
    u64 unmoved_pawns = state.pawn_unmoved & state.variant_pawns();
    fen += ' ';
    if(!unmoved_pawns)
      fen += '-';
    while(unmoved_pawns)
      fen += Notation::square_to_coordinates[pop_ls1b(unmoved_pawns)];
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

  for(const int move : move_list) {
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
