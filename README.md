# Maharajah

C++ chess engine with UCI support and a C interface for app integrations, ported from
MaharajahC. The app, the site and the server use a copy of it, `Maharajah_ffi/src` in the
Maharajah monorepo. Besides standard chess it plays **compound (fairy) pieces** throughout move
generation, SEE, evaluation and draw rules:

- `A` / `a` — Archbishop (bishop + knight)
- `C` / `c` — Chancellor (rook + knight)
- `M` / `m` — Amazon / "Maharajah" (queen + knight)

Variant rules are decided per side: a variant side may promote to A/C/M, double-step each
pawn once from its start square, and never castles. FEN takes two optional extra fields:

- field 7 — variant rights: `V` for White, `v` for Black, `-` for neither. Without it,
  a side is classic unless it holds a compound piece.
- field 8 — the variant side's pawns that have never moved (e.g. `c3e3`, or `-`). It is
  the only source of a variant pawn's double step: with `-` or without the field none may
  double-step. A classic side's pawns are never listed and double-step from rank 2 / 7.

Search: iterative deepening PVS with aspiration windows, a lockless transposition table
shared by Lazy SMP threads, null-move pruning (only with at least two pieces besides king
and pawns, against zugzwang), reverse futility pruning, internal iterative reduction, check
extension, late-move reductions, futility and late-move pruning, killer/history ordering and
SEE-pruned quiescence. Strength is set with `difficulty 1..5` (the level shown to players)
or the lower-level `skill 1..10`.

Evaluation: an NNUE network compiled into the engine (`nets/net5.nnue`: 18 piece kinds ×
64 squares → 768 × 2 perspectives → SCReLU → 1, the compound pieces as inputs of their
own), trained only on the engine's own self-play. The classic hand-written evaluation
remains as a fallback.

## Credits

The engine is based on **BBC (Bitboard Chess Engine)** by Maksim Korzh, specifically the
reference version [`bbc_1.2.c`](https://github.com/maksimKorzh/bbc/blob/master/src/old_versions/bbc_1.2.c),
and on his video series
[Bitboard CHESS ENGINE in C](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs).

The path: BBC 1.2 → [MaharajahC](https://github.com/Villi14/MaharajahC) → this C++ port → NNUE.
The bit-packed move encoding, magic bitboards, MVV-LVA / killer / history ordering, PVS,
null-move pruning, LMR and the aspiration window all originate in BBC. Added on top: the
compound pieces, the per-side variant rules, the self-trained NNUE, Lazy SMP and C++23.

The SIMD row add, subtract and copy of the NNUE accumulator
([headers/NnueSimd.h](headers/NnueSimd.h)) are adapted from `simd.h` of the
[nnue library](https://github.com/jdart1/nnue) by Jon Dart, author of the Arasan chess engine.

## Build Commands

```sh
cmake -S . -B build                          # Release by default
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug # asserts enabled
cmake -S . -B build -DMAHARAJAH_SANITIZE=ON  # ASan + UBSan
cmake --build build
```

### Linux ARM64 (cross-compiled)

With the `aarch64-linux-gnu` toolchain (`g++-aarch64-linux-gnu` on Debian/Ubuntu):

```sh
cmake -S . -B build-linux-arm64 -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
      -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
cmake --build build-linux-arm64 -j
```

The NNUE uses NEON there. The tests are aarch64 binaries too, so they run on an ARM64
machine (or under `qemu-aarch64`).

### Linux x64 (cross-compiled)

With the `x86_64-linux-gnu` toolchain (`g++-x86-64-linux-gnu` on Debian/Ubuntu), e.g. on
an ARM64 host:

```sh
cmake -S . -B build-linux-x64 -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
      -DCMAKE_CXX_COMPILER=x86_64-linux-gnu-g++ \
      -DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST
cmake --build build-linux-x64 -j
```

`PRE_TEST` lists the tests when `ctest` runs instead of right after the build, which
would run the x64 test binary on the build host. The NNUE uses SSE2 by default; add
`-DCMAKE_CXX_FLAGS=-mavx2` for the AVX2 path (the binaries then need an AVX2 CPU).

### Windows x64

With Visual Studio 2026 (the "Desktop development with C++" workload):

```sh
cmake -S . -B build-x64 -G "Visual Studio 18 2026" -A x64
cmake --build build-x64 --config Release --parallel
./build-x64/tests/Release/MaharajahTESTS.exe
```

The binaries land in `build-x64/sources/Release` (`Maharajah.exe`, `maharajah_ffi.dll`),
`build-x64/tools/Release` and `build-x64/tests/Release`. The NNUE uses SSE2 (MSVC's
`_M_X64`); `-DMAHARAJAH_NATIVE=ON` has no effect with MSVC.

### Windows ARM64

Cross-compiled on an x64 Windows host with Visual Studio 2026 (the "MSVC ARM64 build
tools" component of the Visual Studio Installer):

```sh
cmake -S . -B build-arm64 -G "Visual Studio 18 2026" -A ARM64
cmake --build build-arm64 --config Release --parallel
```

The binaries land in `build-arm64/sources/Release` (`Maharajah.exe`, `maharajah_ffi.dll`),
`build-arm64/tools/Release` and `build-arm64/tests/Release`. The NNUE uses NEON (MSVC's
`_M_ARM64`). An x64 machine cannot run them, so `MaharajahTESTS.exe` is run on the ARM64
device.

### Release binaries

The binaries attached to a GitHub release are built by two scripts, one archive per
target in `dist/`:

```sh
tools/build_release_linux.sh linux-arm64 linux-x64 linux-x64-avx2
tools\build_release_windows.ps1 windows-x64 windows-x64-avx2 windows-arm64   # Developer PowerShell
```

Linux binaries are linked statically, so they run on any distribution; Windows ones use
the static C++ runtime, so they need no Visual C++ Redistributable. `x64` is the SSE2
baseline for any 64-bit x86 CPU, `x64-avx2` needs Haswell / Ryzen or newer
(`-march=x86-64-v3`, `/arch:AVX2`). A target of the host's architecture is tested; another
one is cross-compiled. Each script also runs `go depth 18` from the start position: the
node count must be the same in every build.

## Run

```sh
./build/sources/Maharajah           # UCI engine on stdin/stdout
./build/sources/Maharajah --debug   # prints the board and searches the start position
```

UCI options: `Hash` (4-128 MB), `Skill Level` (1-10), `Threads` (1-64, default 1; more
than one thread searches faster but is no longer deterministic), `EvalFile` (a `.nnue`
file; `<empty>` restores the built-in network) and `UseNNUE` (`false` for the classic
evaluation). `go` accepts `depth`, `movetime`,
`wtime`/`btime`/`winc`/`binc`/`movestogo` and `infinite`; a running search stops on `stop`.

## C interface

`build/sources/libmaharajah_ffi` exports the `mah_*` functions declared in
[headers/ffi/maharajah_ffi.h](headers/ffi/maharajah_ffi.h), the same API as MaharajahC:

1. `mah_init()`
2. `mah_set_position_startpos()` or `mah_set_position_fen(...)` / `mah_set_position_fen_with_rules(...)`
3. (optional) `mah_set_hash_mb(mb)`, `mah_set_difficulty_level(1..5)`, `mah_set_skill_level(1..10)`, `mah_set_threads(1..64)`
4. (loop) `mah_apply_move(...)`, `mah_game_status()`, `mah_get_fen(...)` and
   `mah_best_move_depth(...)` / `mah_best_move_time(...)`
5. `mah_shutdown()`

`mah_analyze_depth(...)` / `mah_analyze_time(...)` search at full strength and also return
the score, mate distance and depth. The network is managed with `mah_set_eval_mode`,
`mah_load_weights` / `mah_load_weights_from_bytes` / `mah_load_default_weights` /
`mah_unload_weights`, and reported by `mah_get_eval_status`, `mah_get_weights_version` and
`mah_get_nnue_info` (JSON).

`mah_generate_custom_position_fen(side, seed, ...)` generates a custom variant start.

## Browser build

`tools/build_wasm.sh [out_dir]` builds `maharajah_engine.js` + `.wasm` with Emscripten
(`emcc` on `PATH`) into `build-wasm/out`. It is a drop-in replacement for the module
MaharajahC builds for the web game: an ES6 factory `createMaharajahEngine` exporting the
same `mah_wasm_*` functions plus `mah_wasm_analyze_time` ([headers/wasm/maharajah_wasm.h](headers/wasm/maharajah_wasm.h)),
which return strings instead of filling buffers and are called through `cwrap`:

```js
const engine = await createMaharajahEngine();
engine.cwrap('mah_wasm_init', 'number', [])();
const bestMoveTime = engine.cwrap('mah_wasm_best_move_time', 'string', ['number']);
```

## maharajah_tool

`./build/tools/maharajah_tool` is a command-line tool that drives the engine through the
`mah_*` functions, as MaharajahC's tool does and with the same output:

```sh
maharajah_tool generate [count] [seed]                               # custom start FENs
maharajah_tool selfplay [games] [depth] [max_plies] [seed] [json_path]
maharajah_tool legalmoves <fen>                                      # one move per line
maharajah_tool bench                                                 # small fixed suite
maharajah_tool uci                                                   # UCI subset over mah_*
```

`uci` understands `uci`, `isready`, `ucinewgame`, `setoption` (`Hash`, `Skill Level`,
`UI Difficulty`, `Threads`), `position` and `go depth|movetime`, plus `status`, `getfen [fullmove]`
and `legalmoves` (ends with `endmoves`). Maharajah_lab uses it as the engine and match
arbiter against Fairy-Stockfish. A `position` command that extends the previous one only
applies the new moves, so the hash table survives between moves.

`python3 tools/ask_engine.py "<fen>" --level 1-5` asks the UCI engine for its move at
the strength and think time the app uses for that level.

## Tests

```sh
ctest --test-dir build --output-on-failure
./build/tests/MaharajahTESTS --gtest_filter=-*_d6     # skip the slow depth-6 perft cases
```

Perft results are checked against <https://www.chessprogramming.org/Perft_Results> and Stockfish (`go perft N`).

## Links

- [Bitboard CHESS ENGINE in C](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs), Maksim Korzh's video series on the Chess Programming channel
- <https://en.wikipedia.org/wiki/Bitboard>
- <https://www.chessprogramming.org/Looking_for_Magics>
- <https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html>
- <https://gekomad.github.io/Cinnamon/BitboardCalculator/>

## License

Released under the [MIT License](LICENSE). The engine derives from the WTFPL-licensed BBC `bbc_1.2.c`
(see [Credits](#credits)); the WTFPL places no restrictions on redistribution, so the derived
work is distributed here under the MIT License.
