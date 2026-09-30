# TODO — port of MaharajahC

Branch `port-maharajahc`. The engine, UCI, C interface (`mah_*`), custom-position
generator, browser wrapper (`mah_wasm_*`), `maharajah_tool` and `ask_engine.py` are
ported from MaharajahC; the engine plays identical moves.

The reference C engine is `Maharajah/Maharajah_ffi/src` in the monorepo (the one the app
ships). `../MaharajahC` is a copy of it and must be kept in sync: port a change there
first, then here. Last sync 2026-09-30 (monorepo 54ef8d0, pawn-wall army generator);
`src/`, `include/`, `tests/`, `CMakeLists.txt` and `tools/maharajah_tool.c` are
identical.

## Status (2026-09-30)

- 273 tests pass in ctest (also under ASan/UBSan): 267 gtest cases (63 of them ported
  from MaharajahC's `tests/*_smoke.c`, 5 for `mah_wasm_*`), `uci_smoke.sh` and 5
  `maharajah_tool` tests.
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
- [x] Point Maharajah_lab at the C++ `maharajah_tool` (`MAHARAJAH_TOOL_BIN`) and rerun
      its FSF legal-move spike and an Elo match (2026-09-30): `flutter test test/fsf
      test/elo` passes (47, none skipped; legal moves identical to FSF in all 8 variant
      positions). `bash lab elo --levels=1,2,3,4,5 --opponents=elo:1350 --games=6
      --movetime=200`, C++ vs C: L1 +1=4-1 / +2=3-1, L2 +4-2 / +4-2, L3 +5-1 / +6,
      L4 and L5 +6 for both. Same strength within noise (timed games, 6 per level);
      a larger run with a stronger anchor (elo:1800) would separate L3-L5.
      Follow-up on the 3882 positions of those games: C and C++ FFI, fresh engine per
      search, all 5 levels (L5 at depth 8) — 0 of 19410 moves differ. At `go movetime
      100` both use the same time; C++ finishes one more iteration in ~12% of positions
      and then plays a different move in ~1.3%. Head-to-head C++ vs C at L5, 100 ms/move,
      531 start positions from those games, colours swapped: +338 =461 -263, +25 Elo
      [+15, +34] (95%, clustered by start). The gain is speed only; L1-L4 are
      depth-capped and play identically.
- [x] Port the browser wrapper `src/wasm/maharajah_wasm.c` (2026-09-30):
      `sources/wasm/MaharajahWasm.cpp`, `tools/build_wasm.sh` (Emscripten through CMake,
      target `maharajah_engine`), `WasmApiTests.cpp` (native), `tools/compare_wasm.mjs`.
      Against a fresh C module from Maharajah_ffi (emcc 6.0.10): 8432 checks over the 35
      positions of `compare_engines.py` (depths 1-7, 80-ply self-play at depth 4, FEN,
      status, rejected input) — 0 mismatches. Fixed depths in Node: C++ 1-14 % faster.
      Differences from the C build: `-fwasm-exceptions` (the FEN parser throws), no
      `-flto` (with it emcc 6.0.10 never catches the exception), `-sSTACK_SIZE=1MB`
      (per-ply move lists live on the stack; the 64 KB default overflows). The module
      is 319 KB of wasm (C: 91 KB). Not yet deployed: the web game
      (`Maharajah_net/Maharajah.Api/wwwroot/assets/wasm`) still ships the C module,
      built 2026-08-01, before the pawn-wall sync.
- [ ] Add a `perft` UCI command (both engines lack one) and compare perft counts for
      variant positions.
- [ ] Compare timed searches (`go movetime`, `mah_best_move_time`) — only fixed-depth
      searches are compared exactly; timed ones depend on the clock.
- [ ] Hook the C++ `maharajah_ffi` library into the app instead of the C one and
      check it on the target platforms (Android page size flag is set in CMake).
- [ ] Run the tests on Windows/MSVC (CI covers Ubuntu and macOS only); the stdin
      polling there uses `PeekNamedPipe` and is untested. The `maharajah_tool` shell
      smoke tests are skipped on Windows.

## Engine improvements (proposed 2026-09-30)

Suggested order: match runner, TT move, history, tapered eval; one commit each, each
backed by a match.

- [x] Parity decided (2026-09-30): identical moves were only a check that the port
      broke nothing, not a goal. Strength changes may diverge from MaharajahC and are
      not ported back; `compare_engines.py` stays useful for refactors that must not
      change the search (run it against the previous C++ build instead).
- [x] The C engine is the fixed strength baseline (2026-09-30): MaharajahC `6e2a0f7`
      = monorepo `54ef8d0`, UCI binary `../MaharajahC/build-compare/Maharajah`.
      Every improvement is measured against it too, so the total gain since the port
      stays visible. Maharajah_ffi may still change for the app; the baseline stays
      pinned to this commit, tagged `baseline-2026-09-30` in MaharajahC.
- [ ] Match runner with SPRT (`tools/match.py`): two binaries, parallel games, opening
      book, SPRT stop, results logged against the C baseline. cutechess/fastchess do
      not know the A/C/M pieces, so it needs its own arbiter (`maharajah_tool uci` has
      `status`/`getfen`). The C UCI engine has no `Threads` option and ignores
      `Skill Level` (see the bug list), so baseline matches run at full strength,
      one thread each.
- [ ] Store the best move in the TT and search it first. The entry has 8 spare
      bytes (`padding`), so it stays 24 bytes. Consider depth-preferred replacement
      instead of always-replace.
- [ ] Bound the history heuristic (`Search.cpp`, `history_moves_ +=
      history_bonus_scale * depth * depth`): no cap, no malus, no aging, so on deep
      searches a quiet move can outscore the killers (8000-9000) and even captures
      (10000+). Use a gravity update with a limit and a malus for quiet moves that
      did not cut.
- [ ] Tapered evaluation: `Evaluator::evaluate` picks one of three phases by
      thresholds, so the score jumps when a trade crosses one; interpolate between the
      opening and endgame terms by phase instead.
- [ ] Modern reductions: log(depth)·log(move) LMR table instead of a fixed one-ply
      reduction; adaptive null move R = 3 + depth/4 instead of 2; internal iterative
      reduction when there is no TT move.
- [ ] Quiescence: generate captures only (it now generates all moves and skips the
      quiet ones) and probe the TT.
- [ ] Smaller: fail-soft instead of fail-hard; widen the aspiration window gradually
      instead of jumping to a full window; soft/hard time limits (do not start an
      iteration after ~50% of the budget).
- [ ] Texel-tune the evaluation parameters on self-play positions (they are
      hand-set now).
- [x] Lazy SMP (2026-09-30): `run_search` in `Search.cpp`, UCI/tool option `Threads`
      (1-64, default 1), `mah_set_threads`. Helpers search copies of the board and
      share a lockless TT; one thread is unchanged (0 mismatches in
      `compare_engines.py` and `compare_wasm.mjs`). Clean under TSan. Apple M2 Pro:
      1.27 / 2.47 / 4.87 / 9.61 MN/s at 1/2/4/8 threads. Match 4 threads vs 1, 100
      ms/move, 20 openings x 2 colours x 3: +60 =24 -36, +70 Elo [+15, +129] (95%).
- [ ] Lazy SMP follow-ups: a persistent thread pool instead of threads per `go`;
      threads in the WASM build (`-pthread`, SharedArrayBuffer, COOP/COEP headers).
- [ ] Later: NNUE (the `Nnue` class is a stub).

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
