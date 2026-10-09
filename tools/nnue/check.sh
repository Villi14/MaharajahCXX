#!/usr/bin/env bash
# Prints what the round scripts depend on, before a night run: binaries and their build
# type, the networks, the background generator's command line (depth, share), the data
# files with their position counts, memory, disk and turbo. Changes nothing.
# Usage: tools/nnue/check.sh
set -uo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

echo "== binaries"
for f in "$TEXEL" "$TRAIN" "$TOOL"; do
  if [[ -x "$f" ]]; then echo "ok      $f"; else echo "MISSING $f"; fi
done
cache="$BIN_DIR/../CMakeCache.txt"
[[ -f "$cache" ]] && echo "build type: $(sed -n 's/^CMAKE_BUILD_TYPE:STRING=//p' "$cache")"

echo; echo "== networks ($NETS_DIR)"
ls -l "$NETS_DIR"/*.nnue 2>/dev/null

echo; echo "== nnue-gen (round 4 settings: depth and custom share are in ExecStart)"
systemctl is-active nnue-gen 2>/dev/null
systemctl cat nnue-gen 2>/dev/null | grep -E 'ExecStart|WorkingDirectory'

echo; echo "== data (lines = positions)"
for dir in "$NNUE_DATA" "$NNUE_DATA/r4" "$NNUE_DATA/r5"; do
  [[ -d "$dir" ]] || continue
  files=("$dir"/*.txt)
  [[ -e "${files[0]}" ]] || continue
  echo "$dir: ${#files[@]} files"
  wc -l "${files[@]}" | awk '{ printf "  %12d  %s\n", $1, $2 }' | tail -n 60
done

echo; echo "== machine"
grep -E 'MemTotal|MemAvailable' /proc/meminfo
df -h "$NNUE_DATA" | tail -1
echo "cores: $(nproc)"
turbo=/sys/devices/system/cpu/intel_pstate/no_turbo
[[ -f "$turbo" ]] && echo "no_turbo = $(cat "$turbo")"
