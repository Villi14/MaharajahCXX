#include "../headers/Game.h"
#include <iostream>

using namespace std;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    maharajah::Game game;
    game.play();
    const int score = game.shutdown();
    cout << "Game State = " << game.state() << '\n';
    cout << "Score = " << score << '\n';
  } catch(...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
