#include "../headers/Game.h"

#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
  // Unbuffered stdin lets a running search see "stop" as soon as it arrives.
  setvbuf(stdin, nullptr, _IONBF, 0);
  cin.tie(nullptr);

  const bool debug = argc > 1 && strcmp(argv[1], "--debug") == 0;

  try {
    maharajah::Game game;
    game.play(debug);
    game.shutdown();
  } catch(...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
