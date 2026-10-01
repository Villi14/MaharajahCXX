# TODO — port of MaharajahC

Branch `port-maharajahc`, squashed into one commit on 2026-10-01. The commit hashes
below (43464a7, f702335, ...) are in the unsquashed history, tag
`port-maharajahc-history`. The engine, UCI, C interface (`mah_*`), custom-position
generator, browser wrapper (`mah_wasm_*`), `maharajah_tool` and `ask_engine.py` are
ported from MaharajahC; the engine plays identical moves.

The reference C engine is `Maharajah/Maharajah_ffi/src` in the monorepo (the one the app
ships). `../MaharajahC` is a copy of it and must be kept in sync: port a change there
first, then here. Last sync 2026-10-01 (monorepo 9f841a1, one double step per pawn);
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

## Plan (2026-10-01)

1. Small tasks on this branch, one commit each (a match where strength can change):
   the UCI binary clears the TT only on `ucinewgame`; soft/hard time limits in
   clock mode; a `perft` UCI command with variant perft counts. Done 2026-10-01
   (85b789f, 7b9b9d9, ff8b10b); the two UCI changes gained about +94 and +105 Elo
   on a clock.
2. Confirm the gain: HEAD against the port (43464a7) and the C baseline with
   `match.py`, and the elo:2200 Fairy-Stockfish run on this machine.
3. Review the branch and merge it into `master`.
4. NNUE in a new branch from `master`.

## To do

- [ ] Review the branch and merge it into `master`.
- [x] Variant pawns double-stepped more than once (2026-10-01): without FEN field 8
      a variant side's pawns could double-step from any rank, any number of times
      (a4-a6, then a6-a8 without promoting, stuck on the last rank). The rule is one
      double step per pawn, from its start square on its own half. A position with a
      variant side now always tracks its unmoved pawns (`BoardState::infer_pawn_state`:
      without field 8, a variant side's pawns on its own half and a standard side's on
      its home rank count as unmoved); the army generator writes field 8.
      Same fix in the monorepo's Maharajah_ffi (9f841a1) and MaharajahC (e235bff);
      `legalmoves` of both engines agree on 300 generated armies and the test
      positions. The baseline binary `build-compare/` (tag `baseline-2026-09-30`)
      keeps the old rule. Not yet rebuilt: the app's FFI library and the web
      game's wasm module (Maharajah_ui writes field 8, so the app should not hit it).
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
- [x] `go perft N` in the UCI binary (2026-10-01; MaharajahC lacks it): per-move
      counts and `Nodes searched`, as in Stockfish. `tools/compare_perft.py` against
      Fairy-Stockfish `c19b5f6` (Maharajah_lab `fsf/variants.ini`): 14 hand positions
      (3 orthodox, 11 both-sides variant: compound armies, promotion to A/C/M with
      and without capture, en passant, knight checks) at depths 1-5 and 34 generated
      armies at depths 1-4 — 0 mismatches in 206 counts. 166 generated armies are
      outside what FSF can represent (pawns unmoved off the home rank, mixed sides).
      Five variant cases added to `PerftTests.cpp`.
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

- [x] Absolute strength vs Fairy-Stockfish (2026-10-01): `bash lab elo --levels=5
      --opponents=elo:2200 --games=40 --movetime=200` in Maharajah_lab, run under
      `caffeinate -i`, 1 thread, Apple M2 Pro:
      C++ (Release, 399c0b4) +16 =2 -22, 42.5 %, -53 Elo [-162, +47] ≈ 2150 FSF-Elo;
      C baseline (Maharajah_ffi `maharajah_tool`, 54ef8d0) +10 =2 -28, 27.5 %,
      -168 Elo [-308, -67] ≈ 2030 FSF-Elo. Reports in Maharajah_lab
      `build/elo/2026-10-01T08-55-44-027900/` (C++) and `…T08-39-20-617567/` (C).
      40 games are too few to separate the two (intervals overlap); the head-to-head
      +25 Elo above is the better C++/C comparison. Point `MAHARAJAH_TOOL_BIN` at a
      Release build in its own directory (`build-release/`): VS Code's CMake Tools
      reconfigures `build/` as Debug, and two runs with that -O0 binary scored
      -301 and -382 Elo.
      Rerun on the Ubuntu machine (2026-10-01, Xeon E5-2680 v4, 1 thread per engine,
      24 matches in parallel, 520 ms/move for both sides ≈ 200 ms on the M2 Pro: one
      core here searches ~490 kN/s, the M2 Pro ~1300), 12 x 40 games per engine
      (the 20 openings each played 24 times, so the intervals are a little narrow):
      C++ f702335 +227 =37 -216, 51.1 %, +8 Elo [-22, +38] ≈ 2210 FSF-Elo;
      C baseline +152 =38 -290, 35.6 %, -103 Elo [-135, -72] ≈ 2100 FSF-Elo.
      C++ over C ≈ +110 Elo, in line with the +92 of the direct match. FSF `c19b5f6`
      built with `make build ARCH=x86-64-modern largeboards=yes` in the monorepo's
      `third_party/fairy-stockfish`; `bin/elo_match.dart` is not in the monorepo, so
      `lib/elo_match.dart` was compiled with `dart compile exe` and run directly.
      Next: the match runner below.
      Strength check before the merge (2026-10-01, same setup, 520 ms/move; the C
      baseline is not rerun, it is known at ~2080-2100): HEAD 0d94f7c alone, 24 x 40
      games, +394 =74 -492, -36 [-57, -14] ≈ 2164. Control, f702335 and HEAD side by
      side, 12 x 40 games each: f702335 +200 =41 -239, -28 [-58, +1] ≈ 2172; HEAD
      +223 =40 -217, +4 [-25, +34] ≈ 2204. The same binary moves by ~40 Elo between
      runs, so the gain since f702335 (fail-soft, aspiration widening, ≈ +38 in
      self-play) is below what these runs resolve. Pooled: HEAD ≈ 2177 (1440 games),
      f702335 ≈ 2191 (960). The lab plays through `maharajah_tool`, so the two UCI
      gains (TT kept, clock limits) do not show here.

- [x] Parity decided (2026-09-30): identical moves were only a check that the port
      broke nothing, not a goal. Strength changes may diverge from MaharajahC and are
      not ported back; `compare_engines.py` stays useful for refactors that must not
      change the search (run it against the previous C++ build instead).
- [x] The C engine is the fixed strength baseline (2026-09-30): MaharajahC `6e2a0f7`
      = monorepo `54ef8d0`, UCI binary `../MaharajahC/build-compare/Maharajah`.
      Every improvement is measured against it too, so the total gain since the port
      stays visible. Maharajah_ffi may still change for the app; the baseline stays
      pinned to this commit, tagged `baseline-2026-09-30` in MaharajahC.
- [~] Match runner (`tools/match.py`, 2026-10-01): first version — two binaries,
      parallel games, each opening (start position + 4-8 random plies, or a generated
      custom army + 0-2 plies) with both colours, adjudication through a third
      `maharajah_tool` (`status`/`getfen`/`legalmoves`, threefold, 50 moves,
      bare kings, 400 plies), Elo with a 95 % interval from the game pairs. 200
      openings x 2 at 50 ms/move take ~5 min on the M2 Pro (8 games in parallel).
      Still open: SPRT stop, a fixed opening book, logging against the C baseline
      (`../MaharajahC/build-compare/maharajah_tool` works as BASE). The `--tc` clock
      mode (UCI binaries, `go wtime/btime`) is written but not yet run.
- [x] Results 2026-10-01, each build against the one before, 200 openings x 2
      colours, 50 ms/move, 1 thread, Release (M2 Pro):

      | change | commit | result | Elo [95 %] |
      |---|---|---|---|
      | unused includes removed (clangd include cleaner) | c9f2d26 | — | — |
      | best move in the TT, searched first | 212e7b9 | +202 =32 -166 | +31 [+1, +62] |
      | bounded history (gravity, malus, cutoffs only) | e05d855 | +396 =55 -349 (800) | +20 [~0, +42] |
      | tapered evaluation over the whole phase range | 2c95e17 | +189 =26 -185 | +3 [-26, +33] |
      | log(depth)·log(move) LMR table | 2fcd0e9 | +211 =43 -146 | +57 [+28, +87] |
      | null move R = 2 + depth/6, non-PV, eval >= beta | a8b5819 | +201 =34 -165 | +31 [+3, +60] |

      | quiescence generates captures only (same tree, ~10 % faster) | 57c06f5 | — | — |
      | internal iterative reduction (depth >= 4, not root, no TT move → depth - 1) | after 57c06f5 | +578 =108 -514 (1200) | ≈ +19 [~+1, +36] |
      | fail-soft (search, quiescence, RFP, null move, TT bounds) | after 6903997 | +1171 =213 -1016 (2400, Ubuntu) | ≈ +22 [+9, +35] |
      | aspiration fail: failed bound = fail-soft score ∓ 50, 100, 200, … | after 100086e | +1154 =203 -1043 (2400, Ubuntu) | ≈ +16 [+3, +29] |

      Sum of the single matches ≈ +140 Elo. Measured in one match on the Ubuntu
      machine (2026-10-01, 28 cores, 12 games in parallel, 50 ms/move, 600 games):
      c9f2d26..a8b5819 (HEAD before 57c06f5) vs the port (43464a7) +355 =51 -194,
      +96 Elo [+71, +122]; vs the C baseline (`../MaharajahC/build-compare/
      maharajah_tool`) +348 =60 -192, +92 Elo [+67, +119]. Freeze the binaries
      (copy them out of `build-release/`) before a match: rebuilding during a match
      swaps the engine under the games still to start. Nodes to the fixed
      depths of `bench_engines.py`: 6.67 M → 1.65 M. Rejected: null move R = 3 +
      depth/4 (-5 [-33, +23]); TT probe in the quiescence search (cutoff at
      non-PV nodes, hash move first; -9 % nodes): +10 over 1200 games against
      57c06f5, and -18 [-43, +7] on top of IIR; again on top of fail-soft and
      the aspiration widening (b9a869a, same nodes): +539 =93 -568, ≈ -8 over 1200. Tapered eval was neutral but kept (continuous, needed
      for tuning).
- [x] Store the best move in the TT and search it first (212e7b9). The move shares
      the data word (score 24 bits, depth 8, flag 2, move 26); still 24 bytes.
- [~] Depth-preferred / aged TT replacement (2026-10-01): no clear gain at 50 ms/move,
      not committed. Generation counter in bits 60-63, bumped once per `run_search`;
      1200 games each against f702335 (Ubuntu, 2 x 12 games in parallel,
      `match.py --hash`):

      | variant | Hash 64 MB | Hash 4 MB |
      |---|---|---|
      | one slot: keep another position's entry if deeper and from this search | +577 =114 -509, ≈ +20 | +543 =91 -566, ≈ -7 |
      | buckets of 2 (depth-preferred + always-replace), same 24-byte entries | +512 =99 -589, ≈ -22 | +563 =110 -527, ≈ +10 |

      Intervals about ±17 Elo. Both point the wrong way (the gain should show where
      the table fills, at 4 MB), so it reads as noise around zero. The bucket build
      searches as fast as the base (~490 kN/s, same nodes to the `bench_engines.py`
      depths). `maharajah_tool uci` keeps the table between moves, `position`
      in the UCI binary clears it. Retried at 200 ms/move on top of b9a869a (fail-soft,
      aspiration widening), one-slot variant, Hash 64 MB, 1200 games: seed 1
      +258 =56 -286 (-16 [-40, +8]), seed 2 +262 =64 -274 (-7 [-29, +15]), together
      ≈ -12. Rejected again; always-replace stays.
- [x] Bound the history heuristic (e05d855).
- [x] Tapered evaluation (2c95e17).
- [x] LMR table (2fcd0e9) and adaptive null move (a8b5819).
- [x] Internal iterative reduction (2026-10-01): nodes to the fixed depths of
      `bench_engines.py` 1.65 M → 1.29 M.
- [x] Quiescence generates captures only (57c06f5). Probing the TT there was
      rejected twice (see the results above, also with fail-soft); retry once the
      TT keeps deeper entries.
- [x] Fail-soft instead of fail-hard (2026-10-01): negamax and quiescence return the
      best score, reverse futility the static evaluation, null move its score (not
      a mate), the TT a bound's stored score. Root move scores stay `max(score,
      alpha)`, so the weak skill levels choose among the same near-best moves.
      Nodes to the `bench_engines.py` depths 1.29 M → 0.95 M. Four 600-game
      matches against 6903997 (seeds 1-4): -1, +42, +17, +32.
- [x] Soft/hard time limits in clock mode (`go wtime/btime`, `Game::parse_go`,
      2026-10-01). The share of the clock (time / movestogo + increment) is a target:
      no iteration starts after 60 % of it, a running one may go on to 2.5 times it,
      but not past 40 % of the time left. Before, the search stopped at the target and
      threw the unfinished iteration away. Mean time per move on 42 positions at
      2+0.02: 77 ms (before 89, target 86). UCI binaries at 2+0.02 against 85b789f:
      seed 1 +372 =37 -191 (+108 [+85, +133]), seed 2 +373 =27 -200 (+103 [+79,
      +129]). `go movetime` and `mah_best_move_time` keep a fixed time; there the
      gain would be using the best move of an unfinished iteration.
- [x] Gradual aspiration widening (2026-10-01). First try, fail-hard: no gain. On a fail
      only the failed bound moves out, by 50, 100, 200, ... up to the full window
      (before: straight to a full window). Nodes to the `bench_engines.py` depths
      1.293 M → 1.308 M (few fails there). 1200 games against 09ffdd7, 50 ms/move:
      seed 1 +283 =58 -259 (+14 [-8, +36]), seed 2 +256 =58 -286 (-17 [-41, +6]),
      together +539 =116 -545, ≈ -2. On top of fail-soft the failed bound moves past
      the returned score instead (same doubling steps): seeds 1-4 +16, +9, +24,
      +16, see the table. Nodes to the `bench_engines.py` depths 0.95 M → 0.92 M.
- [x] UCI binary: `Game` cleared the TT on every `position` command (also in
      MaharajahC), so in a UCI game each move started with an empty table. Now it is
      cleared only on `ucinewgame`; the app path (`mah_*`, `maharajah_tool uci`)
      already kept it. UCI binaries on a clock (`match.py --tc 2+0.02`, Ubuntu,
      2 x 12 games in parallel) against 4d78437: seed 1 +376 =10 -214 (+96 [+68,
      +125]), seed 2 +371 =13 -216 (+92 [+64, +121]). Only UCI play gains (GUIs,
      FSF matches); the app and the `maharajah_tool` matches are unchanged.
- [~] Texel-tune the evaluation parameters on self-play positions (2026-10-01):
      first fit no gain, weights not committed. `Evaluation::weights` now holds one
      `EvalWeights` per phase and `tools/texel.cpp` (`maharajah_texel gen` / `tune`)
      fits them (bf474f3, bfb117a). Data: 12 000 self-play games at depth 8 from the
      start position + 6-10 random plies, and 5000 from custom armies + 0-3 plies;
      wins adjudicated at +-1000 for 6 plies, draws at +-15 for 12 plies from ply 80.
      1.84 M positions, 1.44 M quiet (quiescence = static eval). All 2 x 415 weights
      fitted by Adam, K 0.93, error 0.1177 → 0.1096 (converged by 1000 epochs).
      The fit inflated opening material (Q 1420, M 1904) and shrank endgame material
      (P 60, N 172), and turned some signs (endgame rook open file -23, knight
      mobility -8). 1200 games against c36ed54, 50 ms/move: seed 1 +262 =44 -294
      (-19 [-42, +5]), seed 2 +277 =31 -292 (-9 [-33, +16]), together ≈ -14.
      To try: fewer weights (material and the scalars, tables fixed), a penalty
      towards the current values, more and more balanced data (no random-ply
      openings that are lost from the start, higher depth).
- [ ] NNUE: adapt https://github.com/jdart1/nnue (planned for the Ubuntu machine);
      the `Nnue` class is a stub, `EvalMode::nnue` and `mah_load_weights*` exist.
      After the merge into `master`, in its own branch (see the plan above).
      `maharajah_texel gen` writes self-play positions with results (FEN;result),
      a start for training data.
- [x] Lazy SMP (2026-09-30): `run_search` in `Search.cpp`, UCI/tool option `Threads`
      (1-64, default 1), `mah_set_threads`. Helpers search copies of the board and
      share a lockless TT; one thread is unchanged (0 mismatches in
      `compare_engines.py` and `compare_wasm.mjs`). Clean under TSan. Apple M2 Pro:
      1.27 / 2.47 / 4.87 / 9.61 MN/s at 1/2/4/8 threads. Match 4 threads vs 1, 100
      ms/move, 20 openings x 2 colours x 3: +60 =24 -36, +70 Elo [+15, +129] (95%).
- [ ] Lazy SMP follow-ups: a persistent thread pool instead of threads per `go`;
      threads in the WASM build (`-pthread`, SharedArrayBuffer, COOP/COEP headers).

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
