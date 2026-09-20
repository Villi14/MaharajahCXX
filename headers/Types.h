#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace maharajah {

using u64 = uint64_t;

constexpr u64 zero{ 0x0 };
constexpr u64 one{ 0x1 };

enum Colors : int { white, black, both };
enum Pieces : int { P, N, B, R, Q, K, p, n, b, r, q, k, no_pieces };
enum Castling : int { wk = 0b1, wq = 0b10, bk = 0b100, bq = 0b1000 };
enum class Sliders : int { rook, bishop };
enum class TypeMove : int { all_moves, only_captures };
// clang-format off
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
// clang-format on

[[nodiscard]] inline constexpr Squares to_square(const int value) {
  return static_cast<Squares>(value);
}

[[nodiscard]] inline Squares operator+(const Squares square, const int offset) {
  int value = static_cast<int>(square) + offset;

  if(value < a8 || value >= no_square)
    return no_square;

  return to_square(value);
}

[[nodiscard]] inline Squares operator-(const Squares square, const int offset) {
  int value = static_cast<int>(square) - offset;

  if(value < a8 || value >= no_square)
    return no_square;

  return to_square(value);
}

inline Squares& operator++(Squares& square) {
  int value = static_cast<int>(square) + 1;

  if(value >= no_square)
    value = no_square;

  square = to_square(value);
  return square;
}

inline Squares operator++(Squares& square, int) {
  Squares old = square;
  ++square;
  return old;
}

[[nodiscard]] inline constexpr Pieces to_piece(const int value) {
  return static_cast<Pieces>(value);
}

inline Pieces& operator++(Pieces& piece) {
  int value = static_cast<int>(piece) + 1;

  if(value >= no_pieces)
    value = no_pieces;

  piece = to_piece(value);
  return piece;
}

inline Pieces operator++(Pieces& piece, int) {
  Pieces old = piece;
  ++piece;
  return old;
}

} // namespace maharajah
