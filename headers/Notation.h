#pragma once

#include "Types.h"

namespace maharajah {

struct Notation {
  // convert ASCII character pieces to encoded constants
  static constexpr std::array<char, 128> char_pieces = [] {
    std::array<char, 128> t{ }; // zero-init

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
  static constexpr std::array<char, 128> promoted_pieces = [] {
    std::array<char, 128> t{ }; // '\0' by default

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

  // clang-format off
  static constexpr std::array<const char*, 64> square_to_coordinates{
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
  };
  

#ifdef _MSC_VER
  static constexpr std::array<const char*, 12> display_pieces{ 
    "P", "N", "B", "R", "Q", "K", "p", "n", "b", "r", "q", "k" 
  };
#else
  static constexpr std::array<const char*, 12> display_pieces{ 
    "♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚" 
  };
#endif
  // clang-format on
};

} // namespace maharajah
