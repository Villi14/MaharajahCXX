#pragma once

#include "../headers/Types.h"

#include <array>
#include <string_view>

namespace maharajah {

// FEN strings used only by the tests (Fen::start_position lives in Constants.h).
struct TestFen {
  static constexpr std::string_view empty_board{ "8/8/8/8/8/8/8/8 w - - 0 1" };
  static constexpr std::string_view tricky_position{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" };
  static constexpr std::string_view killer_position{ "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1" };
  static constexpr std::string_view cmk_position{ "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9" };
};

// Piece bitboards of the initial position.
struct StartBitboards {
  static constexpr u64 black_pawns{ 0xFF00 };
  static constexpr u64 black_knights{ 0x42 };
  static constexpr u64 black_bishops{ 0x24 };
  static constexpr u64 black_rooks{ 0x81 };
  static constexpr u64 black_queen{ 0x8 };
  static constexpr u64 black_king{ 0x10 };
  static constexpr u64 white_pawns{ 0xFF000000000000 };
  static constexpr u64 white_knights{ 0x4200000000000000 };
  static constexpr u64 white_bishops{ 0x2400000000000000 };
  static constexpr u64 white_rooks{ 0x8100000000000000 };
  static constexpr u64 white_queen{ 0x800000000000000 };
  static constexpr u64 white_king{ 0x1000000000000000 };

  // Occupancy of each side in the initial position, indexed by Colors (white, black).
  static constexpr std::array<u64, 2> occupancy{
    white_pawns | white_knights | white_bishops | white_rooks | white_queen | white_king,
    black_pawns | black_knights | black_bishops | black_rooks | black_queen | black_king,
  };
};

} // namespace maharajah
