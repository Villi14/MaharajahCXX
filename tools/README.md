# Tools

- `compare_engines.py` — plays identical UCI and FFI sessions against MaharajahC and
  this engine and diffs every `info` line (score, depth, nodes, PV) and `bestmove`.
- `bench_engines.py` — search speed of both engines on the same positions and depths
  (the node counts match, so the time is directly comparable).
- `maharajah_tool.cpp` — command-line tool over the `mah_*` functions (`generate`,
  `selfplay`, `legalmoves`, `bench`, `uci`), a port of MaharajahC's
  `tools/maharajah_tool.c`; see the main [README](../README.md#maharajah_tool). Its
  smoke tests are in `tests/`.
- `build_wasm.sh` — builds the browser module (`mah_wasm_*`) with Emscripten.
- `compare_wasm.mjs` — loads MaharajahC's and this engine's browser modules in Node and
  diffs the results of the same `mah_wasm_*` calls.
- `match.py` — head-to-head match between two builds (parallel games, each opening
  with both colours, Elo with a 95 % interval). Strength changes are measured with it,
  e.g. `python3 tools/match.py new/maharajah_tool old/maharajah_tool --pairs 200
  --movetime 50` (~5 min on an M2 Pro); use Release builds.
- `texel.cpp` (`maharajah_texel`) — Texel tuning of `Evaluation::weights`.
  `maharajah_texel gen data.txt 12000 8 28` plays self-play games at depth 8 on 28
  threads and writes `FEN;result` lines; `maharajah_texel tune data.txt 2000 28 out.txt`
  fits the weights to the results on the quiet positions and writes a `weights` block
  in the layout of `headers/Evaluation.h` (with 0 epochs, the current one unchanged).
- `ask_engine.py` — asks the UCI engine for its move in a position at an app difficulty
  level.

## Setup

MaharajahC in `../MaharajahC` is a copy of the engine the app ships,
`../Maharajah/Maharajah_ffi/src`, and has to match it before comparing
(`diff -rq ../MaharajahC/src ../Maharajah/Maharajah_ffi/src/src`, same for `include`).

Build this project and MaharajahC, both in Release:

```sh
CXXFLAGS= cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

C=../MaharajahC
cmake -S $C -B $C/build-compare -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_C_FLAGS=-include $PWD/$C/include/engine/Zobrist.h -include $PWD/$C/include/engine/Attacks.h"
cmake --build $C/build-compare -j --target Maharajah maharajah_ffi
```

The `-include` flags work around missing includes in MaharajahC's
`src/ffi/maharajah_ffi.c`, which current clang rejects.

Other build locations can be given with `MAHARAJAH_C_BUILD` and `MAHARAJAH_CXX_BUILD`.

## Run

```sh
python3 tools/compare_engines.py --quick   # ~5 s
python3 tools/compare_engines.py           # full run, several minutes; exit code 1 on a mismatch
python3 tools/bench_engines.py 5           # 5 repeats per position, median, ~1 min
```

To compare `maharajah_tool` with MaharajahC's, build `--target maharajah_tool` in
`$C/build-compare` too and diff the outputs; everything but the timing fields and the
`id name` version must match:

```sh
T=../MaharajahC/build-compare/maharajah_tool
diff <($T generate 300 1) <(build/tools/maharajah_tool generate 300 1)
diff <($T selfplay 6 3 120 7) <(build/tools/maharajah_tool selfplay 6 3 120 7)
diff <($T legalmoves "<fen>") <(build/tools/maharajah_tool legalmoves "<fen>")
```

Positions are sent as `position fen <FEN>` without a `moves` list, because MaharajahC
misreads `moves` as extra FEN fields. Skill levels are compared through the FFI, as
MaharajahC ignores the UCI `Skill Level` option.

## Browser module

Build this engine's module with `tools/build_wasm.sh`, and MaharajahC's with the flags
of `Maharajah_ffi/tool/build_wasm.sh` but a different output (that script writes into
Maharajah_net's `wwwroot`), plus the same `-include` workaround:

```sh
E=../Maharajah/Maharajah_ffi/src; OUT=/tmp/c-wasm; mkdir -p $OUT
emcc $(find $E/src -name '*.c' ! -path '*/cli/main.c') -I$E/include \
  -include $E/include/engine/Zobrist.h -include $E/include/engine/Attacks.h \
  -O3 -flto -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -sMODULARIZE=1 \
  -sEXPORT_ES6=1 -sEXPORT_NAME=createMaharajahEngine -sENVIRONMENT=web,node \
  -sNO_EXIT_RUNTIME=1 -sEXPORTED_RUNTIME_METHODS=cwrap \
  -sEXPORTED_FUNCTIONS=_mah_wasm_init,_mah_wasm_set_position_fen,_mah_wasm_apply_move,_mah_wasm_game_status,_mah_wasm_get_fen,_mah_wasm_best_move_depth,_mah_wasm_best_move_time \
  -o $OUT/maharajah_engine.js
node tools/compare_wasm.mjs $OUT/maharajah_engine.js build-wasm/out/maharajah_engine.js [--quick]
```

It uses the position lists of `compare_engines.py`.
