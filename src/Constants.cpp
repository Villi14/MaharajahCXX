#include "Constants.h"

namespace m {

	u64 board[2]{
		black_pawns | black_knights | black_bishops | black_rooks | black_queen | black_king,
		white_pawns | white_knights | white_bishops | white_rooks | white_queen | white_king
	};

	const char* square_to_coordinates[]{
		"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
		"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
		"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
		"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
		"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
		"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
		"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
	};

	const int bishop_relevant_bits[]{
		6, 5, 5, 5, 5, 5, 5, 6,
		5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5,
		6, 5, 5, 5, 5, 5, 5, 6
	};

	const int rook_relevant_bits[]{
		12, 11, 11, 11, 11, 11, 11, 12,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		12, 11, 11, 11, 11, 11, 11, 12
	};

	const char* empty_board{ "8/8/8/8/8/8/8/8 w - - " };
	const char* start_position{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 " };
	const char* tricky_position{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 " };
	const char* killer_position{ "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1" };
	const char* cmk_position{ "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 " };


	const std::string ascii_pieces{ "PNBRQKpnbrqk" };

	const std::vector<std::string> unicode_pieces{
		"♙", "♘", "♗", "♖", "♕", "♔",
		"♟︎", "♞", "♝", "♜", "♛", "♚"
	};

	const std::map<char, pieces> char_pieces{
		{'P', P}, {'N', N}, {'B', B}, {'R', R}, {'Q', Q}, {'K', K},
		{'p', p}, {'n', n}, {'b', b}, {'r', r}, {'q', q}, {'k', k}
	};

} // namespace m