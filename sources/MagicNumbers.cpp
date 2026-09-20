#include "../headers/Bitboard.h"
#include "../headers/tables/MagicNumbersTable.h"
#include <algorithm>
#include <random>
#include <vector>

using namespace std;

namespace maharajah {

inline u64 random_u64(mt19937_64& rng) {
  return rng();
}

inline u64 random_magic(mt19937_64& rng) {
  return random_u64(rng) & random_u64(rng) & random_u64(rng);
}

inline bool good_magic_candidate(const u64 mask, const u64 magic) {
  return count_bits((mask * magic) & 0xFF00000000000000ULL) >= 6;
}

u64 find_magic_number(const Squares square, const int relevant_bits, const Sliders figure) {
  const u64 attack_mask = figure == Sliders::bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);

  const int size = 1 << relevant_bits;

  vector<u64> occupancies(size);
  vector<u64> attacks(size);
  vector<u64> used(size);

  for(int i = 0; i < size; ++i) {
    occupancies[i] = set_occupancy(i, relevant_bits, attack_mask);
    attacks[i] = figure == Sliders::bishop ? bishop_attacks_on_the_fly(square, occupancies[i]) : rook_attacks_on_the_fly(square, occupancies[i]);
  }

  mt19937_64 rng(0x9e3779b97f4a7c15ULL);

  for(int attempt = 0; attempt < 10'000'000; ++attempt) {
    const u64 magic = random_magic(rng);

    if(!good_magic_candidate(attack_mask, magic))
      continue;

    ranges::fill(used, zero);

    bool fail = false;

    for(int i = 0; i < size; ++i) {
      if(const int index = static_cast<int>((occupancies[i] * magic) >> (64 - relevant_bits)); used[index] == zero)
        used[index] = attacks[i];
      else if(used[index] != attacks[i]) {
        fail = true;
        break;
      }
    }

    if(!fail)
      return magic;
  }

  return 0ULL;
}

#if 0
void init_magic_numbers() {
  for(Squares square{ a8 }; square < no_square; ++square) {
   rook_magic_numbers[square] = find_magic_number(square, MagicTables::rook_relevant_bits[square], sliders::rook);
   bishop_magic_numbers[square] = find_magic_number(square, MagicTables::bishop_relevant_bits[square], sliders::bishop);
  }
}
#endif

} // namespace maharajah
