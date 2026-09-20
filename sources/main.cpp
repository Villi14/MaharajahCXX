#include "../headers/Game.h"
#include "../headers/Perft.h"
#include <iostream>

using namespace std;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  try {
    maharajah::Game game;
    game.play();
    game.shutdown();
  } catch(...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
