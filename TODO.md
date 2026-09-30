# TODO — port of MaharajahC

Branch `port-maharajahc`. The engine, UCI, C interface (`mah_*`), custom-position
generator, `maharajah_tool` and `ask_engine.py` are ported from MaharajahC; the engine
plays identical moves.

The reference C engine is `Maharajah/Maharajah_ffi/src` in the monorepo (the one the app
ships). `../MaharajahC` is a copy of it and must be kept in sync: port a change there
first, then here. Last sync 2026-09-30 (monorepo 54ef8d0, pawn-wall army generator);
`src/`, `include/`, `tests/`, `CMakeLists.txt` and `tools/maharajah_tool.c` are
identical.

## Status (2026-09-30)

- 268 tests pass in ctest (also under ASan/UBSan): 262 gtest cases (63 of them ported
  from MaharajahC's `tests/*_smoke.c`), `uci_smoke.sh` and 5 `maharajah_tool` tests.
- `maharajah_tool` output matches the C tool's apart from timings and the `id name`
  version (0.1.0 vs 0.2.1): `generate` (300 FENs), `bench` (moves and node counts),
  `selfplay` (6 games at depth 3, text and JSON), `legalmoves` (1832 positions from 30
  self-play games plus standard ones) and a scripted `uci` session.
- `tools/compare_engines.py` (full run, 2026-09-30, after the generator sync): 2047
  searches, 13 887 `info` lines — 0
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
- [x] Port `maharajah_tool` (`tools/maharajah_tool.cpp`, smoke tests in
      `tools/tests/`) and `ask_engine.py` (2026-09-30).
- [x] Port MaharajahC's smoke tests to gtest (2026-09-30): `RulesTests.cpp`
      (compound_piece, engine_rules, special_moves, pawn_unmoved), `DrawRulesTests.cpp`,
      `EvaluationTests.cpp` (evaluate_safety, see), `EngineConfigTests.cpp`
      (engine_config, transposition), `CustomSetupTests.cpp`, `SearchSanityTests.cpp`,
      `FfiApiTests.cpp`; `uci_smoke.sh` runs as is. `perft_smoke.c` is covered by
      `PerftTests.cpp` (deeper). FENs kept verbatim.
- [ ] Not ported, probably not needed: `tools/probe_capture.c` (ad-hoc king-capture
      debug probe, not built by CMake); `Maharajah_ffi/tool/ffi_smoke.dart` belongs
      with the app hook-up below.
- [ ] Point Maharajah_lab at the C++ `maharajah_tool` (`MAHARAJAH_TOOL_BIN`) and rerun
      its FSF legal-move spike and an Elo match.
- [ ] Port the browser wrapper `src/wasm/maharajah_wasm.c` (7 `mah_wasm_*` functions
      returning strings instead of filling buffers), built with Emscripten by
      `Maharajah_ffi/tool/build_wasm.sh` for the web game. Needed before the C++
      engine can replace the C one in the browser.
- [ ] Add a `perft` UCI command (both engines lack one) and compare perft counts for
      variant positions.
- [ ] Compare timed searches (`go movetime`, `mah_best_move_time`) — only fixed-depth
      searches are compared exactly; timed ones depend on the clock.
- [ ] Hook the C++ `maharajah_ffi` library into the app instead of the C one and
      check it on the target platforms (Android page size flag is set in CMake).
- [ ] Run the tests on Windows/MSVC (CI covers Ubuntu and macOS only); the stdin
      polling there uses `PeekNamedPipe` and is untested. The `maharajah_tool` shell
      smoke tests are skipped on Windows.

## Bugs found in MaharajahC (not fixed there)

- [ ] `setoption name Skill Level value N` never applies: `strncmp(input, "setoption
      name Skill Level value ", 34)` compares 34 characters of a 33-character literal.
- [ ] `position fen <FEN> moves ...`: the parser reads `moves` as FEN field 7 and the
      first move as field 8 (unmoved pawns), so variant rights are lost and pawn double
      steps break. The C++ port splits off `moves` before parsing.
- [ ] Does not build with current clang (C23): missing `#include`s —
      `Zobrist.h`/`Attacks.h` in `src/ffi/maharajah_ffi.c`, `Utils.h` in
      `tools/maharajah_tool.c`.
- [ ] `tests/see_smoke.c` uses a malformed FEN, `8/1k6/1b6/3N3/8/8/8/4K3` (7 squares
      on rank 5); the lenient parser shifts the ranks below, putting the white king on
      e2. The port uses `3N4`.
- [ ] `parse_go`: the tiny-increment branch checks `depth == 64` before a missing
      depth is defaulted to 64, so it only fires for an explicit `go depth 64`
      (kept for parity in `Game::parse_go`).

## Differences from MaharajahC (intended)

- `id name` reports this project's version (`Maharajah 0.1.0`, C: `0.2.1`), set once in
  the root `CMakeLists.txt`. On `uci` the options are listed again (C repeats only
  name, author and `uciok`).

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
