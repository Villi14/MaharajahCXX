#!/usr/bin/env bash
# Generates a data round for a time budget: `maharajah_texel gen` in chunks of
# GAMES_PER_CHUNK games, $NNUE_DATA/$ROUND/chunk_NNNN.txt, until HOURS are over. Reruns
# continue after the last chunk. The chunk running at the deadline is stopped and keeps
# its complete lines (the trainer aborts on a cut-off line).
#
# Usage: tools/nnue/gen.sh HOURS
#   ROUND=r5 NET=net5a DEPTH=7 CUSTOM_SHARE=0.65 THREADS=24 GAMES_PER_CHUNK=2000
#   SEED_BASE=50000 PREPARE=1 (stop nnue-gen, fans, turbo off)
#   WIN_SCORE=1000: a side at >= this for 6 plies is adjudicated the winner; 0 plays
#   every game out (won endgames converted in the data, fewer positions per hour)
#   WIDE_SHARE=0 MIXED_SHARE=0: games from variant armies of any shape (0-27 pawns,
#   swarms of one piece type) and from the orthodox army against a variant one; the
#   WALL_SHARE=0: a pawn wall (1-3 solid ranks) with a swarm of knights, bishops, rooks
#   or queens behind it, against the orthodox army, another wall or any variant army.
#   ANY_SHARE=0: any of the 2580 armies a player can build (1-3 maharajahs, 1-4 queens,
#   pawnless armies, ...) against the orthodox army, its mirror image or another army.
#   The five shares add up to at most 1, the rest starts from the start position.
#   Round r6 (NET6_ENDGAME.md): CUSTOM_SHARE=0 WIDE_SHARE=0.35 MIXED_SHARE=0.45 WIN_SCORE=0
# Run it in tmux: it asks for sudo once at the start.
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

HOURS="${1:-}"
[[ "$HOURS" =~ ^[0-9]+([.][0-9]+)?$ ]] || die "usage: $0 HOURS"
ROUND="${ROUND:-r5}"
NET="$(net_path "${NET:-net5a}")"
DEPTH="${DEPTH:-7}"
CUSTOM_SHARE="${CUSTOM_SHARE:-0.65}"
GAMES_PER_CHUNK="${GAMES_PER_CHUNK:-2000}"
SEED_BASE="${SEED_BASE:-50000}"
WIN_SCORE="${WIN_SCORE:-1000}"
WIDE_SHARE="${WIDE_SHARE:-0}"
MIXED_SHARE="${MIXED_SHARE:-0}"
WALL_SHARE="${WALL_SHARE:-0}"
ANY_SHARE="${ANY_SHARE:-0}"

need_file "$TEXEL"
check_release
OUT_DIR="$NNUE_DATA/$ROUND"
mkdir -p "$OUT_DIR/log"
[[ "${PREPARE:-1}" == 1 ]] && prepare_machine

deadline=$(( $(date +%s) + $(awk -v h="$HOURS" 'BEGIN { printf "%d", h * 3600 }') ))
start=$(date +%s)
echo "round $ROUND: teacher $NET, depth $DEPTH, custom share $CUSTOM_SHARE, wide share $WIDE_SHARE, mixed share $MIXED_SHARE, wall share $WALL_SHARE, any share $ANY_SHARE, win score $WIN_SCORE, $THREADS threads,"
echo "  $GAMES_PER_CHUNK games per chunk, until $(date -d "@$deadline" '+%F %T')"

# the next free chunk number
chunk=0
while [[ -e "$(printf '%s/chunk_%04d.txt' "$OUT_DIR" "$chunk")" ]]; do chunk=$((chunk + 1)); done

written=0
while :; do
  remaining=$(( deadline - $(date +%s) ))
  (( remaining > 120 )) || break
  name="$(printf 'chunk_%04d' "$chunk")"
  part="$OUT_DIR/$name.part"
  chunk_start=$(date +%s)
  set +e
  timeout --signal=TERM "$remaining" "$TEXEL" gen "$part" "$GAMES_PER_CHUNK" "$DEPTH" "$THREADS" \
    "$((SEED_BASE + chunk))" "$CUSTOM_SHARE" "$NET" "$WIN_SCORE" "$WIDE_SHARE" "$MIXED_SHARE" "$WALL_SHARE" "$ANY_SHARE" 2>"$OUT_DIR/log/$name.log"
  rc=$?
  set -e
  if (( rc != 0 && rc != 124 )); then
    tail -5 "$OUT_DIR/log/$name.log" >&2
    die "maharajah_texel gen failed ($rc) on $name"
  fi
  # whole lines only: FEN;score;result
  grep -E '^[^;]+;-?[0-9]+;(0\.0|0\.5|1\.0)$' "$part" >"$OUT_DIR/$name.txt" || true
  rm -f "$part"
  lines=$(wc -l <"$OUT_DIR/$name.txt")
  written=$((written + lines))
  took=$(( $(date +%s) - chunk_start ))
  echo "$(date '+%T') $name: $lines positions in ${took}s$( (( rc == 124 )) && echo ' (stopped at the deadline)')"
  chunk=$((chunk + 1))
  (( rc == 124 )) && break
done

hours=$(awk -v s="$(( $(date +%s) - start ))" 'BEGIN { printf "%.2f", s / 3600 }')
echo "done: $written positions in $hours h ($(awk -v n="$written" -v h="$hours" 'BEGIN { printf "%.2f", (h > 0 ? n / h / 1e6 : 0) }') M/h)"
echo "total in $OUT_DIR: $(cat "$OUT_DIR"/chunk_*.txt | wc -l) positions"
