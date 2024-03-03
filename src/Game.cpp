#include "Game.h"
#include "Bitboard.h"
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

using namespace std;

namespace m {

	game::game() : m_game_state_{ start_game }, m_score_{ 0 } { init_all(); }

	game_state game::state() const { return m_game_state_; }

	void game::play() {
		m_game_state_ = play_game;

		//  stringstream ss;
		//  ofstream fs("bishop_occupancy.txt");
		//  for (int square{a8}; square <= h1; square++) {
		//    ss << "{";
		//    U64 attack_mask = mask_bishop_attacks(Squares(square));
		//    for (int index{}; index < 1 << bishop_relevant_bits[square]; index++) {
		//      U64 occupancies = set_occupancy(index, bishop_relevant_bits[square],
		//      attack_mask); ss <<
		//      print_value(rook_attacks_on_the_fly(Squares(square), occupancies));
		//    }
		//    ss << "}," << endl;
		//  }
		//  fs << ss.str();
		//  fs.close();

		parse_fen(
			"r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 w q a3 0 9 ");
		print_board();
		parse_fen(start_position);
		print_board();
		parse_fen(
			"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b Kk e6 0 1 ");
		print_board();
		print_bitboard(occupancies[black]);
		print_bitboard(occupancies[white]);
		print_bitboard(occupancies[both]);
	}

	int game::shutdown() {
		m_game_state_ = end_game;
		return m_score_;
	}

	string game::print_bitboard(const u64 bitboard, const bool only_value) {
		stringstream ss;
		for (int rank{}; rank < rank_bit; rank++) {
			for (int file{}; file < file_bit; file++) {
				int square = rank * rank_bit + file;

				if (!file) {
					string rank_string = to_string(rank_bit - rank);
					ss << format(" {} ", rank_string);
				}
				ss << " ";
				const bool bit = get_bit(bitboard, static_cast<squares>(square));
				ss << (bit ? "1" : "0");
			}
			ss << '\n';
		}
		ss << "\n    a b c d e f g h\n";

		ss << "    Bitboard: ";
		if (only_value)
			ss.str(string());
		ss << "0x" << hex << bitboard << "\n";
		if (only_value)
			ss << ",";
		ss << '\n';
		cout << ss.str();
		return ss.str();
	}

	void game::print_board() {
		stringstream ss;
		ss << "\n";
		for (int rank{}; rank < rank_bit; rank++) {
			for (int file{}; file < file_bit; file++) {
				int square = rank * rank_bit + file;
				if (!file) {
					ss << " ";
					ss << 8 - rank;
					ss << " ";
				}

				int piece{ -1 };

				for (const pieces bb_piece : all_pieces) {
					if (get_bit(bitboards[bb_piece], static_cast<squares>(square)))
						piece = bb_piece;
				}

#ifdef _WIN32
				ss << format(" {}", (piece == -1) ? '.' : ascii_pieces[piece]);
#else
				ss << format(" {}", (piece == -1) ? "." : unicode_pieces[piece].c_str());
#endif
			}
			ss << "\n";
		}

		ss << "\n   a b c d e f g h\n\n";
		ss << format("  Side:     {}\n", side ? "black" : "white");
		ss << format("  Enpassant:   {}\n",
			(enpassant != no_sq) ? square_to_coordinates[enpassant] : "no");
		ss << format("  Castling:  {}{}{}{}\n", (castle & wk) ? 'K' : '-',
			(castle & wq) ? 'Q' : '-', (castle & bk) ? 'k' : '-',
			(castle & bq) ? 'q' : '-');
		ss << "\n";
		cout << ss.str();
	}

	void game::parse_fen(const char* fen) {
		memset(bitboards, empty, sizeof(bitboards));
		memset(occupancies, empty, sizeof(occupancies));

		int side{};
		squares enpassant{ no_sq };
		int castle{};

		for (int rank = 0; rank < rank_bit; rank++) {
			for (int file = 0; file < file_bit; file++) {
				squares square = static_cast<squares>(rank * rank_bit + file);
				if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
					int piece = char_pieces.at(*fen);
					set_bit(bitboards[piece], square);
					fen++;
				}

				if (*fen >= '0' && *fen <= '9') {
					int offset = *fen - '0';
					int piece = -1;

					for (const pieces bb_piece : all_pieces) {
						if (get_bit(bitboards[bb_piece], square))
							piece = bb_piece;
					}

					if (piece == -1)
						file--;

					file += offset;
					fen++;
				}

				if (*fen == '/')
					fen++;
			}
		}

		fen++;
		(*fen == 'w') ? (side = white) : (side = black);
		fen += 2;

		while (*fen != ' ') {
			switch (*fen) {
			case 'K':
				castle |= wk;
				break;
			case 'Q':
				castle |= wq;
				break;
			case 'k':
				castle |= bk;
				break;
			case 'q':
				castle |= bq;
				break;
			case '-':
				break;
			}
			fen++;
		}

		fen++;

		if (*fen != '-') {
			int file = fen[0] - 'a';
			int rank = rank_bit - (fen[1] - '0');
			enpassant = static_cast<squares>(rank * rank_bit + file);
		}
		else {
			enpassant = no_sq;
		}

		for (int piece = P; piece <= K; piece++)
			occupancies[white] |= bitboards[piece];

		for (int piece = p; piece <= k; piece++)
			occupancies[black] |= bitboards[piece];

		occupancies[both] |= occupancies[white];
		occupancies[both] |= occupancies[black];
	}

} // namespace m
