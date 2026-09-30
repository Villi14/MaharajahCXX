# Tools

- `compare_engines.py` — plays identical UCI and FFI sessions against MaharajahC and
  this engine and diffs every `info` line (score, depth, nodes, PV) and `bestmove`.
- `bench_engines.py` — search speed of both engines on the same positions and depths
  (the node counts match, so the time is directly comparable).

## Setup

Build this project and MaharajahC (next to it, in `../MaharajahC`), both in Release:

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

Positions are sent as `position fen <FEN>` without a `moves` list, because MaharajahC
misreads `moves` as extra FEN fields. Skill levels are compared through the FFI, as
MaharajahC ignores the UCI `Skill Level` option.
