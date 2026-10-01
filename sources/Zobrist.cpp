#include "../headers/Zobrist.h"
#include "../headers/Bitboard.h"

namespace maharajah {

const ZobristKeys& ZobristKeys::get() {
  static const ZobristKeys keys = [] {
    ZobristKeys generated{ };
    Random random{ };

    for(auto& piece_keys : generated.piece) {
      for(auto& key : piece_keys)
        key = random.next_u64();
    }

    for(auto& key : generated.en_passant)
      key = random.next_u64();

    for(auto& key : generated.castle)
      key = random.next_u64();

    generated.side = random.next_u64();
    generated.random_state = random.state;
    return generated;
  }();

  return keys;
}

u64 generate_hash_key(const BoardState& state) {
  const ZobristKeys& keys = ZobristKeys::get();
  u64 final_key{ };

  for(Pieces piece{ P }; piece <= k; ++piece) {
    u64 bitboard = state.bitboards[piece];
    while(bitboard) {
      const Squares square = get_ls1b_index(bitboard);
      final_key ^= keys.piece[piece][square];
      pop_bit(bitboard, square);
    }
  }

  if(state.en_passant != no_square)
    final_key ^= keys.en_passant[state.en_passant];

  final_key ^= keys.castle[state.castle];

  if(state.side == black)
    final_key ^= keys.side;

  return final_key;
}

} // namespace maharajah
