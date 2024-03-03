#include <cstring>
#include "Bitboard.h"
#include "MagicNumbers.h"

namespace m {

	u_int random_state{ 0x6B8B4567 };

	int side;
	int enpassant{ no_sq };
	int castle;
	u64 bitboards[12];
	u64 occupancies[3];
	u64 pawn_attacks[2][end_bit];
	u64 knight_attacks[end_bit];
	u64 king_attacks[end_bit];
	u64 bishop_masks[end_bit];
	u64 rook_masks[end_bit];
	u64 bishop_attacks[end_bit][bishop_attacks_count];
	u64 rook_attacks[end_bit][rook_attacks_count];

	u64 mask_pawn_attacks(const colors side, const squares square) {
		u64 attacks = empty;
		u64 bitboard = empty;

		set_bit(bitboard, square);

		if (side == white) {
			if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
			if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
		}
		else {
			if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
			if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
		}

		return attacks;
	}

	u64 mask_knight_attacks(const squares square) {
		u64 attacks{ empty };
		u64 bitboard{ empty };

		set_bit(bitboard, square);

		if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
		if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
		if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
		if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
		if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
		if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
		if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
		if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

		return attacks;
	}

	u64 mask_king_attacks(const squares square) {
		u64 attacks{ empty };
		u64 bitboard{ empty };

		set_bit(bitboard, square);

		if (bitboard >> 8) attacks |= (bitboard >> 8);
		if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
		if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
		if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
		if (bitboard << 8) attacks |= (bitboard << 8);
		if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
		if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
		if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

		return attacks;
	}

	u64 mask_bishop_attacks(const squares square) {
		u64 attacks{ empty };
		int rank, file;
		const int target_rank = square / rank_bit;
		const int target_file = square % file_bit;

		for (rank = target_rank + 1, file = target_file + 1; rank <= 6 && file <= 6; rank++, file++) attacks |= (one << (rank * file_bit + file));
		for (rank = target_rank - 1, file = target_file + 1; rank >= 1 && file <= 6; rank--, file++) attacks |= (one << (rank * file_bit + file));
		for (rank = target_rank + 1, file = target_file - 1; rank <= 6 && file >= 1; rank++, file--) attacks |= (one << (rank * file_bit + file));
		for (rank = target_rank - 1, file = target_file - 1; rank >= 1 && file >= 1; rank--, file--) attacks |= (one << (rank * file_bit + file));

		return attacks;
	}

	u64 mask_rook_attacks(const squares square) {
		u64 attacks{ empty };
		int rank, file;
		const int target_rank = square / rank_bit;
		const int target_file = square % file_bit;

		for (rank = target_rank + 1; rank <= 6; rank++) attacks |= (one << (rank * file_bit + target_file));
		for (rank = target_rank - 1; rank >= 1; rank--) attacks |= (one << (rank * file_bit + target_file));
		for (file = target_file + 1; file <= 6; file++) attacks |= (one << (target_rank * file_bit + file));
		for (file = target_file - 1; file >= 1; file--) attacks |= (one << (target_rank * file_bit + file));

		return attacks;
	}

	u64 bishop_attacks_on_the_fly(const squares square, const u64 block) {
		u64 attacks{ empty };
		int rank, file;
		const int target_rank = square / rank_bit;
		const int target_file = square % file_bit;

		for (rank = target_rank + 1, file = target_file + 1; rank <= 7 && file <= 7; rank++, file++) {
			attacks |= (one << (rank * file_bit + file));
			if ((one << (rank * file_bit + file)) & block) break;
		}

		for (rank = target_rank - 1, file = target_file + 1; rank >= 0 && file <= 7; rank--, file++) {
			attacks |= (one << (rank * file_bit + file));
			if ((one << (rank * file_bit + file)) & block) break;
		}

		for (rank = target_rank + 1, file = target_file - 1; rank <= 7 && file >= 0; rank++, file--) {
			attacks |= (one << (rank * file_bit + file));
			if ((one << (rank * file_bit + file)) & block) break;
		}

		for (rank = target_rank - 1, file = target_file - 1; rank >= 0 && file >= 0; rank--, file--) {
			attacks |= (one << (rank * file_bit + file));
			if ((one << (rank * file_bit + file)) & block) break;
		}

		return attacks;
	}

	u64 rook_attacks_on_the_fly(const squares square, const u64 block) {
		u64 attacks{ empty };
		int rank, file;
		const int target_rank = square / rank_bit;
		const int target_file = square % file_bit;

		for (rank = target_rank + 1; rank <= 7; rank++) {
			attacks |= (one << (rank * file_bit + target_file));
			if ((one << (rank * file_bit + target_file)) & block) break;
		}

		for (rank = target_rank - 1; rank >= 0; rank--) {
			attacks |= (one << (rank * file_bit + target_file));
			if ((one << (rank * file_bit + target_file)) & block) break;
		}

		for (file = target_file + 1; file <= 7; file++) {
			attacks |= (one << (target_rank * file_bit + file));
			if ((one << (target_rank * file_bit + file)) & block) break;
		}

		for (file = target_file - 1; file >= 0; file--) {
			attacks |= (one << (target_rank * file_bit + file));
			if ((one << (target_rank * file_bit + file)) & block) break;
		}

		return attacks;
	}

	void init_all() {
		init_leapers_attacks();
		init_sliders_attacks(bishop);
		init_sliders_attacks(rook);
	}

	void init_leapers_attacks() {
		for (const squares square : all_squares) {
			pawn_attacks[white][square] = mask_pawn_attacks(white, square);
			pawn_attacks[black][square] = mask_pawn_attacks(black, square);
			knight_attacks[square] = mask_knight_attacks(square);
			king_attacks[square] = mask_king_attacks(square);
		}
	}

	u64 set_occupancy(const int index, const int bits_in_mask, u64 attack_mask) {
		u64 occupancy{ empty };

		for (int count{}; count < bits_in_mask; count++) {
			int square = get_ls1b_index(attack_mask);
			pop_bit(attack_mask, static_cast<squares>(square));

			if (index & (one << count))
				occupancy |= (one << square);
		}

		return occupancy;
	}

	u_int get_random_u32_number() {
		u_int number{ random_state };

		number ^= number << 13;
		number ^= number >> 17;
		number ^= number << 5;
		random_state = number;

		return number;
	}

	u64 get_random_u64_number() {
		const u64 n1{ static_cast<u64>(get_random_u32_number()) & 0xFFFF };
		const u64 n2{ static_cast<u64>(get_random_u32_number()) & 0xFFFF };
		const u64 n3{ static_cast<u64>(get_random_u32_number()) & 0xFFFF };
		const u64 n4{ static_cast<u64>(get_random_u32_number()) & 0xFFFF };

		return n1 | (n2 << 0x10) | (n3 << 0x20) | (n4 << 0x30);
	}

	u64 generate_magic_number() {
		return get_random_u64_number() & get_random_u64_number() & get_random_u64_number();
	}

	u64 find_magic_number(const squares square, const int relevant_bits, const bool bishop) {
		u64 occupancies[rook_attacks_count];
		u64 attacks[rook_attacks_count];
		u64 used_attacks[rook_attacks_count];

		const u64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);
		const int occupancy_indices = 1 << relevant_bits;

		for (int index{}; index < occupancy_indices; index++) {
			occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
			attacks[index] = bishop ? bishop_attacks_on_the_fly(square, occupancies[index])
				: rook_attacks_on_the_fly(square, occupancies[index]);
		}

		for (int random_count{}; random_count < 100000000; random_count++) {
			const u64 magic_number{ generate_magic_number() };

			if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6)
				continue;

			memset(used_attacks, empty, sizeof(used_attacks));

			bool fail{ false };
			for (int index{}; !fail && index < occupancy_indices; index++) {
				const int magic_index = static_cast<int>((occupancies[index] * magic_number) >> (end_bit - relevant_bits));

				if (used_attacks[magic_index] == empty)
					used_attacks[magic_index] = attacks[index];
				else if (used_attacks[magic_index] != attacks[index])
					fail = true;
			}

			if (!fail)
				return magic_number;
		}

		return empty;
	}

	void init_magic_numbers() {
		for (const squares square : all_squares) {
			rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
			bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);
		}
	}

	void init_sliders_attacks(const bool bishop) {
		for (const squares square : all_squares) {
			const u64 attack_mask = bishop ? bishop_masks[square] = mask_bishop_attacks(square)
				: rook_masks[square] = mask_rook_attacks(square);
			const int relevant_bits_count = count_bits(attack_mask);
			const int occupancy_indices = (1 << relevant_bits_count);

			for (int index{}; index < occupancy_indices; index++) {
				if (bishop) {
					const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
					const u64 magic_index = (occupancy * bishop_magic_numbers[square]) >> (end_bit - bishop_relevant_bits[square]);
					bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
				}
				else {
					const u64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
					const u64 magic_index = (occupancy * rook_magic_numbers[square]) >> (end_bit - rook_relevant_bits[square]);
					rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
				}
			}
		}
	}

} // namespace m
