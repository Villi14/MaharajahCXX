#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>

#include "Constants.h"

namespace m {

	enum game_state { start_game, play_game, end_game };

	struct game {
		game();
		void play();
		int shutdown();
		game_state state() const;
		static std::string print_bitboard(u64 bitboard, bool only_value = false);
		static std::string print_value(u64 bitboard);
		static void print_board();
		static void parse_fen(const char* fen);

		static std::vector<std::string> m_board;

	private:
		game_state m_game_state_;
		int m_score_;
	};

} // namespace m

#endif // !GAME_H
