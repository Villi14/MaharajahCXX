# Maharajah

## Build Commands

```sh
cmake -S . -B build                          # Release by default
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug # asserts enabled
cmake -S . -B build -DMAHARAJAH_SANITIZE=ON  # ASan + UBSan
cmake --build build
```

Visual Studio: `cmake -G "Visual Studio 18 2026" -S . -B build`.

## Run

```sh
./build/sources/Maharajah           # UCI engine on stdin/stdout
./build/sources/Maharajah --debug   # prints the board and searches the start position
```

## Tests

```sh
ctest --test-dir build --output-on-failure
./build/tests/MaharajahTESTS --gtest_filter=-*_d6     # skip the slow depth-6 perft cases
```

Perft results are checked against <https://www.chessprogramming.org/Perft_Results> and Stockfish (`go perft N`).

## Links

### <https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs>

### <https://en.wikipedia.org/wiki/Bitboard>

### <https://www.chessprogramming.org/Looking_for_Magics>

### <https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html>

### <https://gekomad.github.io/Cinnamon/BitboardCalculator/>
