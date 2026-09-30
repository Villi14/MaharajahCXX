#include "../headers/Engine.h"
#include "../headers/Bitboard.h"

namespace maharajah {

Engine::Engine() {
  static const bool tables_ready = [] {
    AttackTables::init();
    return true;
  }();
  (void)tables_ready;

  // the skill-level random stream continues where hash key generation stopped
  random.state = ZobristKeys::get().random_state;
  board.parse_fen(Fen::start_position);
}

bool Engine::set_position(const std::string_view fen) {
  board.parse_fen(fen);

  // every search path locates the kings, so exactly one per side is required
  if(count_bits(board.state.bitboards[K]) != 1 || count_bits(board.state.bitboards[k]) != 1) {
    board.parse_fen(Fen::start_position);
    return false;
  }

  return true;
}

bool Engine::apply_move(const std::string_view move) {
  const int parsed = board.parse_move(move);
  if(!parsed)
    return false;

  board.remember_position();
  if(!board.make_move(parsed, TypeMove::all_moves)) {
    board.forget_position();
    return false;
  }

  return true;
}

} // namespace maharajah
