#pragma once

#include "../headers/Types.h"

#include <string_view>

namespace maharajah {

// FEN strings used only by the tests (Fen::start_position lives in Constants.h).
struct TestFen {
  static constexpr std::string_view empty_board{ "8/8/8/8/8/8/8/8 w - - 0 1" };
  static constexpr std::string_view tricky_position{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" };
  static constexpr std::string_view killer_position{ "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1" };
  static constexpr std::string_view cmk_position{ "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9" };
};

} // namespace maharajah
