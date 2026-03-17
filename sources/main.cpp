#include "../headers/Game.h"
#include <iostream>

using namespace std;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    maharajah::Game game;
    game.play();
    game.shutdown();
  } catch(...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
