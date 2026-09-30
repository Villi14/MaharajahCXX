# TODO — port of MaharajahC

Branch `port-maharajahc`. The engine, UCI, C interface (`mah_*`) and custom-position
generator are ported from MaharajahC and play identical moves.

## Status (2026-09-30)

- 198 unit tests pass.
- `tools/compare_engines.py` (full run): 1759 searches, 11 835 `info` lines — 0
  mismatches in score, depth, nodes, PV and `bestmove`. Covered: 24 standard and 11
  variant positions at depths 5/7/8, 200 generated custom positions (same FEN per
  seed, same FFI playouts), 7 self-play games of 120 plies, 36 games at skill 1-9 via
  FFI, 12 games from custom positions.
- `tools/bench_engines.py` (Apple M2 Pro, Release, median of 5): C++ is 7-31 %
  faster, 1469 vs 1326 kN/s overall, geometric mean ×1.13.

| position | C kN/s | C++ kN/s | C++/C |
|---|---|---|---|
| start, depth 12 | 1222 | 1312 | 1.07 |
| Kiwipete, 9 | 1137 | 1235 | 1.09 |
| middlegame, 10 | 1188 | 1285 | 1.08 |
| CMK, 10 | 1093 | 1183 | 1.08 |
| pawn endgame, 13 | 2283 | 2993 | 1.31 |
| rook endgame, 13 | 1981 | 2403 | 1.21 |
| A/C/M start, 10 | 1045 | 1141 | 1.09 |
| custom army, 10 | 1450 | 1661 | 1.15 |

## To do

- [ ] Review the branch and merge it into `master`.
- [ ] Decide on the variant pawn quirk (kept for parity): a variant pawn on the 6th
      rank may double-step to the 8th rank without promoting and is stuck there
      (`Board::generate_moves`, same in MaharajahC `Moves.c`). Fix in both engines or
      forbid the double step onto the last rank.
- [ ] Port the remaining tools: `maharajah_tool` (`generate`, `selfplay`, `bench`) and
      `ask_engine.py`.
- [ ] Add a `perft` UCI command (both engines lack one) and compare perft counts for
      variant positions.
- [ ] Compare timed searches (`go movetime`, `mah_best_move_time`) — only fixed-depth
      searches are compared exactly; timed ones depend on the clock.
- [ ] Hook the C++ `maharajah_ffi` library into the app instead of the C one and
      check it on the target platforms (Android page size flag is set in CMake).
- [ ] Run the tests on Windows/MSVC (CI covers Ubuntu and macOS only); the stdin
      polling there uses `PeekNamedPipe` and is untested.

## Bugs found in MaharajahC (not fixed there)

- [ ] `setoption name Skill Level value N` never applies: `strncmp(input, "setoption
      name Skill Level value ", 34)` compares 34 characters of a 33-character literal.
- [ ] `position fen <FEN> moves ...`: the parser reads `moves` as FEN field 7 and the
      first move as field 8 (unmoved pawns), so variant rights are lost and pawn double
      steps break. The C++ port splits off `moves` before parsing.
- [ ] Does not build with current clang (C23): missing `#include`s —
      `Zobrist.h`/`Attacks.h` in `src/ffi/maharajah_ffi.c`, `Utils.h` in
      `tools/maharajah_tool.c`.
- [ ] `parse_go`: the tiny-increment branch checks `depth == 64` before a missing
      depth is defaulted to 64, so it only fires for an explicit `go depth 64`
      (kept for parity in `Game::parse_go`).

## Differences from MaharajahC (intended)

- UCI rejects a position without exactly one king per side and falls back to the
  start position (C searches it and answers `bestmove (none)`).
- Input arriving during a search is read one line at a time, so a command after
  `stop` is not lost; `quit` during a search ends the UCI loop.
- The FEN parser is strict and rejects malformed FENs (C parses leniently).

## Build notes

- The shell environment sets `CXXFLAGS=-I/opt/homebrew/include`, which pulls in
  Homebrew's googletest and breaks the test build. Configure with an empty value:
  `CXXFLAGS= cmake -S . -B build`.
- How to build MaharajahC for the comparison: see [tools/README.md](tools/README.md).
