#!/usr/bin/env bash
# Trains a network on several data rounds: prints the files and position counts, the
# expected memory and time, asks once, then runs maharajah_nnue_train with mirroring.
# Output $NETS_DIR/NAME.nnue (written after every epoch), log $NETS_DIR/NAME.log.
#
# Usage: tools/nnue/train.sh NAME
#   DATA=a.txt,b.txt (instead of the globs below)
#   N3D7_GLOB="$NNUE_DATA/n3d7_s*.txt" R4_GLOB="$NNUE_DATA/r4/*.txt" R5_GLOB="$NNUE_DATA/r5/chunk_*.txt"
#   EPOCHS=20 THREADS=24 LAMBDA=0.75 SEED=1 HIDDEN=768 DECAY=0.3 MIRROR=1 YES=1 PREPARE=1
# The defaults are net5a's recipe (n3d7 + r4) plus round 5.
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

NAME="${1:-}"
[[ -n "$NAME" ]] || die "usage: $0 NAME"
OUT="$NETS_DIR/${NAME%.nnue}.nnue"
LOG="$NETS_DIR/${NAME%.nnue}.log"
[[ ! -e "$OUT" ]] || die "$OUT exists"
EPOCHS="${EPOCHS:-20}" LAMBDA="${LAMBDA:-0.75}" SEED="${SEED:-1}" HIDDEN="${HIDDEN:-768}"
DECAY="${DECAY:-0.3}" MIRROR="${MIRROR:-1}"

need_file "$TRAIN"
check_release

if [[ -n "${DATA:-}" ]]; then
  IFS=, read -r -a files <<<"$DATA"
else
  shopt -s nullglob
  # shellcheck disable=SC2206 # the globs are meant to expand
  files=(${N3D7_GLOB:-$NNUE_DATA/n3d7_s*.txt} ${R4_GLOB:-$NNUE_DATA/r4/*.txt} ${R5_GLOB:-$NNUE_DATA/r5/chunk_*.txt})
  shopt -u nullglob
fi
(( ${#files[@]} > 0 )) || die "no data files"

echo "data:"
total=0
for f in "${files[@]}"; do
  need_file "$f"
  # a file cut off by a stopped generator ends in half a line, on which the trainer aborts
  tail -n 1 "$f" | grep -qE '^[^;]+;-?[0-9]+;(0\.0|0\.5|1\.0)$' ||
    die "$f ends in an incomplete line; trim it: sed -i '\$d' $f"
  n=$(wc -l <"$f")
  total=$((total + n))
  printf '  %12d  %s\n' "$n" "$f"
done
millions=$(awk -v n="$total" 'BEGIN { printf "%.1f", n / 1e6 }')
# measured: 2.3 GB per 17.9 M positions; net5a 966 s per epoch on 44.8 M (24 threads, turbo off)
echo "total $millions M positions; memory ≈ $(awk -v m="$millions" 'BEGIN { printf "%.0f", m * 0.13 + 2 }') GB" \
     "(available $(awk '/MemAvailable/ { printf "%.0f", $2 / 1048576 }' /proc/meminfo) GB);" \
     "time ≈ $(awk -v m="$millions" -v e="$EPOCHS" 'BEGIN { printf "%.1f", m * 21.6 * e / 3600 }') h for $EPOCHS epochs"
# Dataset::first is uint32 over ~34 features a position
(( total < 115000000 )) || die "more than 115 M positions overflow the trainer's uint32 feature index"

echo "$TRAIN -> $OUT: epochs $EPOCHS, threads $THREADS, lambda $LAMBDA, seed $SEED, hidden $HIDDEN, decay $DECAY, mirror $MIRROR"
if [[ "${YES:-0}" != 1 ]]; then
  read -r -p "start? [y/N] " answer
  [[ "$answer" == [yY]* ]] || exit 0
fi
[[ "${PREPARE:-1}" == 1 ]] && prepare_machine

data_list="$(IFS=,; echo "${files[*]}")"
echo "started $(date '+%F %T'), log $LOG"
"$TRAIN" "$data_list" "$OUT" "$EPOCHS" "$THREADS" "$LAMBDA" "$SEED" "$HIDDEN" "$DECAY" "$MIRROR" 2>&1 | tee "$LOG"
echo "finished $(date '+%F %T')"
