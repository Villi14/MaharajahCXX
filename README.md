# Maharajah

C++ chess engine with UCI support and a C interface for app integrations, ported from
MaharajahC. Besides standard chess it plays **compound (fairy) pieces** throughout move
generation, SEE, evaluation and draw rules:

- `A` / `a` — Archbishop (bishop + knight)
- `C` / `c` — Chancellor (rook + knight)
- `M` / `m` — Amazon / "Maharajah" (queen + knight)

Variant rules are decided per side: a variant side may promote to A/C/M, double-step its
pawns from any rank, and never castles. FEN takes two optional extra fields:

- field 7 — variant rights: `V` for White, `v` for Black, `-` for neither. Without it,
  a board with compound pieces plays variant rules for both sides.
- field 8 — squares of pawns that have never moved (e.g. `c3e3`, or `-`). When present
  it decides pawn double steps instead of the rank.

Search: iterative deepening PVS with aspiration windows, transposition table, null-move
pruning, late-move reductions, futility and late-move pruning, killer/history ordering and
SEE-pruned quiescence. Strength is set with `difficulty 1..5` (the level shown to players)
or the lower-level `skill 1..10`.

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

UCI options: `Hash` (4-128 MB) and `Skill Level` (1-10). `go` accepts `depth`, `movetime`,
`wtime`/`btime`/`winc`/`binc`/`movestogo` and `infinite`; a running search stops on `stop`.

## C interface

`build/sources/libmaharajah_ffi` exports the `mah_*` functions declared in
[headers/ffi/maharajah_ffi.h](headers/ffi/maharajah_ffi.h), the same API as MaharajahC:

1. `mah_init()`
2. `mah_set_position_startpos()` or `mah_set_position_fen(...)` / `mah_set_position_fen_with_rules(...)`
3. (optional) `mah_set_hash_mb(mb)`, `mah_set_difficulty_level(1..5)`, `mah_set_skill_level(1..10)`
4. (loop) `mah_apply_move(...)`, `mah_game_status()`, `mah_get_fen(...)` and
   `mah_best_move_depth(...)` / `mah_best_move_time(...)`
5. `mah_shutdown()`

`mah_generate_custom_position_fen(side, seed, ...)` generates a custom variant start.

## Browser build

`tools/build_wasm.sh [out_dir]` builds `maharajah_engine.js` + `.wasm` with Emscripten
(`emcc` on `PATH`) into `build-wasm/out`. It is a drop-in replacement for the module
MaharajahC builds for the web game: an ES6 factory `createMaharajahEngine` exporting the
same `mah_wasm_*` functions ([headers/wasm/maharajah_wasm.h](headers/wasm/maharajah_wasm.h)),
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
`UI Difficulty`), `position` and `go depth|movetime`, plus `status`, `getfen [fullmove]`
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

### <https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs>

### <https://en.wikipedia.org/wiki/Bitboard>

### <https://www.chessprogramming.org/Looking_for_Magics>

### <https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html>

### <https://gekomad.github.io/Cinnamon/BitboardCalculator/>
