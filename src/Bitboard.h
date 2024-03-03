#ifndef BITBOARD_H
#define BITBOARD_H

#include "Constants.h"
#include "MagicNumbers.h"

namespace m {

	extern int side;
	extern int enpassant;
	extern int castle;

	extern u_int random_state;
	extern u64 bitboards[12];
	extern u64 occupancies[3];
	extern u64 pawn_attacks[2][end_bit];
	extern u64 knight_attacks[end_bit];
	extern u64 king_attacks[end_bit];
	extern u64 bishop_masks[end_bit];
	extern u64 rook_masks[end_bit];
	extern u64 bishop_attacks[end_bit][bishop_attacks_count];
	extern u64 rook_attacks[end_bit][rook_attacks_count];
	extern u64 bishop_fly_attacks[end_bit];
	extern u64 rook_fly_attacks[end_bit];

	void init_all();
	void init_leapers_attacks();
	void init_magic_numbers();
	void init_sliders_attacks(bool bishop);

	static u64 get_bishop_attacks(squares square, u64 occupancy);
	static u64 get_rook_attacks(squares square, u64 occupancy);

	u_int get_random_u32_number();
	u64 set_occupancy(int index, int bits_in_mask, u64 attack_mask);
	u64 get_random_u64_number();
	u64 generate_magic_number();
	u64 find_magic_number(squares square, int relevant_bits, bool bishop);
	u64 mask_pawn_attacks(colors side, squares square);
	u64 mask_knight_attacks(squares square);
	u64 mask_king_attacks(squares square);
	u64 mask_bishop_attacks(squares square);
	u64 mask_rook_attacks(squares square);
	u64 bishop_attacks_on_the_fly(squares square, u64 block);
	u64 rook_attacks_on_the_fly(squares square, u64 block);

	inline u64 set_bit(u64& bitboard, const squares square) {
		return bitboard |= (one << square);
	}

	inline bool get_bit(const u64& bitboard, const squares square) {
		return bitboard & (one << square);
	}

	inline u64 pop_bit(u64& bitboard, const squares square) {
		return get_bit(bitboard, square) ? bitboard ^= (one << square) : 0;
	}

	inline int count_bits(u64 bitboard) {
		int count{ 0 };
		while (bitboard) {
			count++;
			bitboard &= bitboard - 1;
		}
		return count;
	}

	inline int get_ls1b_index(const u64 bitboard) {
		if (bitboard) {
			return count_bits((-bitboard & bitboard) - 1);
		}
		return -1;
	}

	inline u64 get_bishop_attacks(const squares square, u64 occupancy) {
		occupancy &= bishop_masks[square];
		occupancy *= bishop_magic_numbers[square];
		occupancy >>= end_bit - bishop_relevant_bits[square];

		return bishop_attacks[square][occupancy];
	}

	inline u64 get_rook_attacks(const squares square, u64 occupancy) {
		occupancy &= rook_masks[square];
		occupancy *= rook_magic_numbers[square];
		occupancy >>= end_bit - rook_relevant_bits[square];

		return rook_attacks[square][occupancy];
	}

} // namespace m

#endif // !BITBOARD_H
