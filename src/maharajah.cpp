#include "Game.h"
#include <iostream>

int main(/* const int argc, const char *argv[] */) {
	try {
		m::game game;
		game.play();
		const int score = game.shutdown();
		std::cout << "Game State = " << game.state() << '\n';
		std::cout << "Score = " << score << '\n';
	}
	catch (...) {
		return 1;
	}

	return EXIT_SUCCESS;
}
