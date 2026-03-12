#include "../headers/Bitboard.h"
#include "../headers/tables/MagicNumbersTable.h"
#include <algorithm>
#include <random>
#include <vector>

using namespace std;

namespace maharajah {

/**
 * @brief Generates a random 64-bit unsigned integer using the given Mersenne Twister.
 *
 * @param rng The Mersenne Twister to use for generating the random number.
 *
 * @return A random 64-bit unsigned integer.
 */
inline u64 random_u64(mt19937_64& rng) {
  return rng();
}

/**
 * @brief Generates a random 64-bit unsigned integer which is likely to be a good magic number candidate.
 * 
 * This function generates three random 64-bit unsigned integers and applies a bitwise AND operation to them.
 * The result is a random number with a high probability of having at least 6 bits set, which is a good starting point for finding magic numbers.
 * 
 * @param rng The Mersenne Twister to use for generating the random number.
 * 
 * @return A random 64-bit unsigned integer which is likely to be a good magic number candidate.
 */
inline u64 random_magic(mt19937_64& rng) {
  return random_u64(rng) & random_u64(rng) & random_u64(rng);
}

/**
 * @brief Checks whether a given magic number is a good candidate for a given mask.
 *
 * A good magic number candidate is defined as a magic number which has at least 6 bits set when
 * multiplied with the given mask and then performing a bitwise AND operation with the magic number.
 *
 * @param mask The mask to check with.
 * @param magic The magic number to check.
 *
 * @return true if the magic number is a good candidate, false otherwise.
 */
inline bool good_magic_candidate(const u64 mask, const u64 magic) {
  return count_bits((mask * magic) & 0xFF00000000000000ULL) >= 6;
}

/**
 * @brief Finds a magic number for a given square and figure.
 *
 * The magic number is found by randomly generating numbers and checking whether they satisfy the following condition:
 * The magic number multiplied with the occupancy of the square and then performing a bitwise AND operation with the attack mask
 * must produce the same result as the attack of the figure on the fly.
 *
 * @param square The square to find the magic number for.
 * @param relevant_bits The number of relevant bits in the magic number.
 * @param figure The figure to find the magic number for (bishop or rook).
 *
 * @return The magic number if found, 0 otherwise.
 */
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
/**
 * @brief Initializes the magic numbers for rooks and bishops.
 *
 * This function is called once at the beginning of the program and is used to
 * initialize the magic numbers for rooks and bishops. The magic numbers are
 * stored in the rook_magic_numbers and bishop_magic_numbers arrays.
 */
void init_magic_numbers() {
  for(Squares square{ a8 }; square < no_square; ++square) {
   rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], sliders::rook);
   bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], sliders::bishop);
  }
}
#endif

} // namespace maharajah
