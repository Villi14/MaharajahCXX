#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace m {

	using u64 = uint64_t;
	using u_int = unsigned int;

	constexpr int end_bit{ 0x40 };
	constexpr int rank_bit{ 0x8 };
	constexpr int file_bit{ 0x8 };
	constexpr int bishop_attacks_count{ 0x200 };
	constexpr int rook_attacks_count{ 0x1000 };

	constexpr u64 empty{ 0x0 };
	constexpr u64 one{ 0x1 };

	constexpr u64 white_pawns{ 0xFF00 };
	constexpr u64 white_knights{ 0x42 };
	constexpr u64 white_bishops{ 0x24 };
	constexpr u64 white_rooks{ 0x81 };
	constexpr u64 white_queen{ 0x8 };
	constexpr u64 white_king{ 0x10 };
	constexpr u64 black_pawns{ 0x00FF000000000000 };
	constexpr u64 black_knights{ 0x4200000000000000 };
	constexpr u64 black_bishops{ 0x2400000000000000 };
	constexpr u64 black_rooks{ 0x8100000000000000 };
	constexpr u64 black_queen{ 0x800000000000000 };
	constexpr u64 black_king{ 0x1000000000000000 };

	constexpr u64 not_a_file{ 0xFEFEFEFEFEFEFEFE };
	constexpr u64 not_h_file{ 0x7F7F7F7F7F7F7F7F };
	constexpr u64 not_hg_file{ 0x3F3F3F3F3F3F3F3F };
	constexpr u64 not_ab_file{ 0xFCFCFCFCFCFCFCFC };

	enum colors : int { white, black, both };

	enum sliders : bool { rook = false, bishop = true };

	enum squares : int {
		a8, b8, c8, d8, e8, f8, g8, h8,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a1, b1, c1, d1, e1, f1, g1, h1, no_sq
	};

	static const squares all_squares[]{
		a8, b8, c8, d8, e8, f8, g8, h8,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a1, b1, c1, d1, e1, f1, g1, h1
	};

	enum pieces { P, N, B, R, Q, K, p, n, b, r, q, k };

	static const pieces all_pieces[]{
		P, N, B, R, Q, K, p, n, b, r, q, k
	};

	enum castling { wk = 0b1, wq = 0b10, bk = 0b100, bq = 0b1000 };

	extern u64 board[];
	extern const char* square_to_coordinates[];
	extern const int bishop_relevant_bits[];
	extern const int rook_relevant_bits[];

	extern const char* empty_board;
	extern const char* start_position;
	extern const char* tricky_position;
	extern const char* killer_position;
	extern const char* cmk_position;

	extern const std::string ascii_pieces;
	extern const std::vector<std::string> unicode_pieces;
	extern const std::map<char, pieces> char_pieces;

} // namespace m

#endif // CONSTANTS_H
