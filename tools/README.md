# Tools

- `compare_engines.py` — plays identical UCI and FFI sessions against MaharajahC and
  this engine and diffs every `info` line (score, depth, nodes, PV) and `bestmove`.
- `bench_engines.py` — search speed of both engines on the same positions and depths
  (the node counts match, so the time is directly comparable).
- `maharajah_tool.cpp` — command-line tool over the `mah_*` functions (`generate`,
  `selfplay`, `legalmoves`, `bench`, `uci`), a port of MaharajahC's
  `tools/maharajah_tool.c`; see the main [README](../README.md#maharajah_tool). Its
  smoke tests are in `tests/`.
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
