#!/usr/bin/env bash
# Builds the Linux release binaries: statically linked (any distribution), one archive
# per target in dist/ (maharajah-<version>-<target>.tar.gz with Maharajah and LICENSE).
# Usage: tools/build_release_linux.sh linux-arm64 linux-x64 linux-x64-avx2
#
# A target of the host's architecture is built natively and its tests run. Another
# architecture is cross-compiled (g++-aarch64-linux-gnu or g++-x86-64-linux-gnu); its
# tests need a machine of that architecture, and the search check runs under
# qemu-<arch> when it is installed.
set -euo pipefail
cd "$(dirname "$0")/.."

if [ $# -eq 0 ]; then
  echo "usage: $0 linux-arm64|linux-x64|linux-x64-avx2 ..." >&2
  exit 1
fi

version=$(sed -n 's/^project(Maharajah VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
host=$(uname -m)
mkdir -p dist

for target in "$@"; do
  # x86-64-v3 (AVX2, BMI2, POPCNT), not -mavx2 alone: std::popcount then is one instruction
  case $target in
    linux-arm64)    arch=aarch64; flags= ;;
    linux-x64)      arch=x86_64;  flags=-march=x86-64 ;;
    linux-x64-avx2) arch=x86_64;  flags=-march=x86-64-v3 ;;
    *) echo "unknown target: $target" >&2; exit 1 ;;
  esac

  cross=()
  if [ "$arch" != "$host" ]; then
    # PRE_TEST: the test list is read when ctest runs, not by running them after the build
    cross=(-DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR="$arch"
           -DCMAKE_CXX_COMPILER="$arch-linux-gnu-g++"
           -DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST)
  fi

  build=build-rel-$target
  rm -rf "$build"
  cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$flags" \
        -DCMAKE_EXE_LINKER_FLAGS=-static "${cross[@]}"
  cmake --build "$build" -j

  engine=$build/sources/Maharajah
  run=()
  if [ "$arch" = "$host" ]; then
    ctest --test-dir "$build" --output-on-failure
  elif command -v "qemu-$arch" > /dev/null; then
    run=("qemu-$arch")
  else
    run=(skip)
    echo "$target: cross-compiled, tests not run (no qemu-$arch)"
  fi

  file "$engine"
  # a fixed-depth search runs to completion; every build must report the same node count
  if [ "${run[0]:-}" != skip ]; then
    printf 'uci\nposition startpos\ngo depth 18\nquit\n' | "${run[@]}" "$engine" |
      grep -E "^id name| depth 18 " | cut -c1-80
  fi

  name=maharajah-$version-$target
  rm -rf "dist/$name"
  mkdir "dist/$name"
  cp "$engine" LICENSE "dist/$name/"
  tar -C dist -czf "dist/$name.tar.gz" "$name"
  rm -rf "dist/$name"
  echo "built dist/$name.tar.gz"
done
