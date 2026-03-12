#pragma once

#include <cstdint>
#include <array>
#include <string_view>

namespace maharajah {

using u64 = uint64_t;
using uint = unsigned int;

constexpr u64 zero{ 0x0 };
constexpr u64 one{ 0x1 };
constexpr int end_bit{ 0x40 };
constexpr int rank_bit{ 0x8 };
constexpr int file_bit{ 0x8 };
constexpr int bishop_attacks_count{ 0x200 };
constexpr int rook_attacks_count{ 0x1000 };

constexpr int count_type_pieces{ 0x6 };
constexpr int white_pawn_index{ 0x0 };
constexpr int black_pawn_index{ 0x1 };
constexpr int knight_index{ 0x2 };
constexpr int bishop_index{ 0x3 };
constexpr int rook_index{ 0x4 };
constexpr int king_index{ 0x5 };

constexpr u64 initial_black_pawns{ 0xFF00 };
constexpr u64 initial_black_knights{ 0x42 };
constexpr u64 initial_black_bishops{ 0x24 };
constexpr u64 initial_black_rooks{ 0x81 };
constexpr u64 initial_black_queen{ 0x8 };
constexpr u64 initial_black_king{ 0x10 };
constexpr u64 initial_white_pawns{ 0xFF000000000000 };
constexpr u64 initial_white_knights{ 0x4200000000000000 };
constexpr u64 initial_white_bishops{ 0x2400000000000000 };
constexpr u64 initial_white_rooks{ 0x8100000000000000 };
constexpr u64 initial_white_queen{ 0x800000000000000 };
constexpr u64 initial_white_king{ 0x1000000000000000 };

constexpr u64 not_a_file{ 0xFEFEFEFEFEFEFEFE };
constexpr u64 not_h_file{ 0x7F7F7F7F7F7F7F7F };
constexpr u64 not_gh_file{ 0x3F3F3F3F3F3F3F3F };
constexpr u64 not_ab_file{ 0xFCFCFCFCFCFCFCFC };

enum Colors : int { white, black, both };
enum Castling : int { wk = 0b1, wq = 0b10, bk = 0b100, bq = 0b1000 };
enum class Sliders : int { rook, bishop};
enum class TypeMove : int { all_moves, only_captures };

enum Squares : int {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1, no_square
};

/**
 * @brief Converts an integer to a Squares enum value.
 *
 * @param value The integer to be converted.
 *
 * @return The corresponding Squares enum value.
 *
 * @note This function is marked as [[nodiscard]] to indicate that it does not modify the program state.
 */
[[nodiscard]] inline constexpr Squares to_square(const int value) {
  return static_cast<Squares>(value);
}

/**
 * @brief Adds an offset to a Squares enum value.
 *
 * @param square The Squares enum value to be offset.
 * @param offset The offset to be added.
 *
 * @return The resulting Squares enum value after adding the offset.
 *
 * @note If the result is out of range (i.e. < a8 or >= no_square), no_square is returned.
 */
[[nodiscard]] inline Squares operator+(const Squares square, const int offset) {
  int value = static_cast<int>(square) + offset;

  if(value < a8 || value >= no_square)
    return no_square;

  return to_square(value);
}

/**
 * @brief Subtracts an offset from a Squares enum value.
 *
 * @param square The Squares enum value to be offset.
 * @param offset The offset to be subtracted.
 * @return The resulting Squares enum value after subtracting the offset.
 * @note If the result is out of range (i.e. < a8 or >= no_square), no_square is returned.
 */
[[nodiscard]] inline Squares operator-(const Squares square, const int offset) {
  int value = static_cast<int>(square) - offset;

  if(value < a8 || value >= no_square)
    return no_square;

  return to_square(value);
}

/**
 * @brief Pre-increment for Squares enum.
 *
 * @param square The Squares enum value to be incremented.
 * @return Reference to the incremented Squares value.
 *
 * @note If the result is out of range (>= no_square), it is clamped to no_square.
 */
inline Squares& operator++(Squares& square) {
  int value = static_cast<int>(square) + 1;

  if(value >= no_square)
    value = no_square;

  square = to_square(value);
  return square;
}

/**
 * @brief Post-increment for Squares enum.
 *
 * @param square The Squares enum value to be incremented.
 * @return The original Squares value before increment.
 *
 * @note If the result is out of range (>= no_square), it is clamped to no_square.
 */
inline Squares operator++(Squares& square, int) {
  Squares old = square;
  ++square;
  return old;
}

enum Pieces : int { P, N, B, R, Q, K, p, n, b, r, q, k, no_pieces };

/**
 * @brief Converts an integer to a Pieces enum value.
 *
 * @param value The integer to be converted.
 *
 * @return The corresponding Pieces enum value.
 *
 * @note This function is marked as [[nodiscard]] to indicate that it does not modify the program state.
 */
[[nodiscard]] inline constexpr Pieces to_piece(const int value) {
  return static_cast<Pieces>(value);
}

/**
 * @brief Pre-increment for Pieces enum.
 *
 * @param piece The Pieces enum value to be incremented.
 * @return Reference to the incremented Pieces value.
 *
 * @note If the result is out of range (>= no_pieces), it is clamped to no_pieces.
 */
 inline Pieces& operator++(Pieces& piece) {
  int value = static_cast<int>(piece) + 1;

  if(value >= no_pieces)
    value = no_pieces;

  piece = to_piece(value);
  return piece;
}

/**
 * @brief Post-increment for Pieces enum.
 *
 * @param piece The Pieces enum value to be incremented.
 * @return The original Pieces value before increment.
 *
 * @note If the result is out of range (>= no_pieces), it is clamped to no_pieces.
 */
inline Pieces operator++(Pieces& piece, int) {
  Pieces old = piece;
  ++piece;
  return old;
}

// convert ASCII character pieces to encoded constants
inline constexpr std::array<char, 128> char_pieces = [] {
  std::array<char, 128> t{}; // zero-init

  t['P'] = P;
  t['N'] = N;
  t['B'] = B;
  t['R'] = R;
  t['Q'] = Q;
  t['K'] = K;

  t['p'] = p;
  t['n'] = n;
  t['b'] = b;
  t['r'] = r;
  t['q'] = q;
  t['k'] = k;

  return t;
}();

// promoted pieces
inline constexpr std::array<char, 128> promoted_pieces = [] {
  std::array<char, 128> t{}; // '\0' by default

  t[Q] = 'q';
  t[R] = 'r';
  t[B] = 'b';
  t[N] = 'n';

  t[q] = 'q';
  t[r] = 'r';
  t[b] = 'b';
  t[n] = 'n';

  return t;
}();

inline constexpr std::array<u64, 2> board {
	initial_white_pawns | initial_white_knights | initial_white_bishops | initial_white_rooks | initial_white_queen | initial_white_king,
  initial_black_pawns | initial_black_knights | initial_black_bishops | initial_black_rooks | initial_black_queen | initial_black_king
};

inline constexpr std::array<const char*, 64> square_to_coord {
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

inline constexpr std::array<int, 64> bishop_relevant_bits {
	6, 5, 5, 5, 5, 5, 5, 6,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 5, 5, 5, 5, 5, 5, 6
};

inline constexpr std::array<int, 64> rook_relevant_bits {
	12, 11, 11, 11, 11, 11, 11, 12,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	12, 11, 11, 11, 11, 11, 11, 12
};

#ifdef _MSC_VER
/** @brief Use ASCII pieces for display on MSVC. */
inline constexpr std::array<const char*, 12> display_pieces { "P", "N", "B", "R", "Q", "K", "p", "n", "b", "r", "q", "k" };
#else
/** @brief Use Unicode pieces for display on other compilers. */
inline constexpr std::array<const char*, 12> display_pieces { "♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚" };
#endif

inline constexpr std::string_view empty_board { "8/8/8/8/8/8/8/8 w - - " };
inline constexpr std::string_view start_position { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 " };
inline constexpr std::string_view tricky_position { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 " };
inline constexpr std::string_view killer_position { "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1" };
inline constexpr std::string_view cmk_position { "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 " };

} // namespace maharajah
