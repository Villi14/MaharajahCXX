#!/usr/bin/env bash
# Builds the browser engine module (maharajah_engine.js + .wasm) with Emscripten,
# the C++ counterpart of Maharajah_ffi/tool/build_wasm.sh.
# Usage: tools/build_wasm.sh [output directory]   (default: build-wasm/out)
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-wasm"
OUT_DIR="${1:-${BUILD_DIR}/out}"

# the shell's CXXFLAGS may point at Homebrew headers, which emcc must not see
CXXFLAGS= emcmake cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j --target maharajah_engine

mkdir -p "${OUT_DIR}"
cp "${BUILD_DIR}/sources/maharajah_engine.js" "${BUILD_DIR}/sources/maharajah_engine.wasm" "${OUT_DIR}/"
echo "WASM engine written to ${OUT_DIR}/maharajah_engine.js/.wasm"
