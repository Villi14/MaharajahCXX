#pragma once

#include "Types.h"

#include <array>
#include <string_view>

namespace maharajah {

struct BoardGeometry {
  static constexpr int squares{ 0x40 };
  static constexpr int ranks{ 0x8 };
  static constexpr int files{ 0x8 };
};

struct Limits {
  static constexpr int max_ply{ 256 };
  static constexpr int max_moves{ 256 };
};

struct FileMask {
  static constexpr u64 not_a_file{ 0xFEFEFEFEFEFEFEFE };
  static constexpr u64 not_h_file{ 0x7F7F7F7F7F7F7F7F };
  static constexpr u64 not_gh_file{ 0x3F3F3F3F3F3F3F3F };
  static constexpr u64 not_ab_file{ 0xFCFCFCFCFCFCFCFC };
};

struct Fen {
  static constexpr std::string_view start_position{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
};

struct CastlingRules {
  /*                        castling   move          in
                              right update     binary  decimal

 king & rooks didn't move:     1111 & 1111  =  1111    15

        white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13

         black king moved:     1111 & 0011  =  0011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7
*/
  // clang-format off
  static constexpr std::array<int, 64> rights{
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
  };
  // clang-format on
};

} // namespace maharajah
