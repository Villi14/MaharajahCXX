#!/usr/bin/env bash
# Maharajah against Stash versions with CCRL Blitz ratings, in cutechess-cli 1.4.0: the run
# behind STRENGTH.md (2026-10-19, cedff82 + net5). Rating of a level = anchor rating + Elo
# difference. Anchor ratings, CCRL Blitz complete list of 2026-10-17 (2'+1" on an i7-4770K):
# 9.0.1 1271, 11.0.1 1687, 12.0 1883, 14.0 2059, 15.3 2174, 17.0 2297, 25.0 2933, 27.0 3049.
#
# Our side is the UCI binary Maharajah, frozen outside the build directory (maharajah_tool
# ignores wtime/btime and thinks 1 s per move, so it loses on time under a clock). App
# levels L1-L5 are Skill Level 2/4/6/8/10. Book 8moves_v3, every opening with both colours.
# PGNs and logs in $ANCHORS/out/tc$TC/<name>.{pgn,log}; results:
#   grep -E '^(Score of|Elo)' $ANCHORS/out/tc60+0.6/*.log
#
# Usage: tools/nnue/ccrl_match.sh l5|ladder|all
#        tools/nnue/ccrl_match.sh one SKILL ANCHOR GAMES   e.g. one 10 stash-27.0 400
#   ANCHORS=$NNUE_DATA/anchors (bin/stash-*, books/8moves_v3.pgn, ours/)
#   OURS=$ANCHORS/ours/maharajah-ea3e6a4-nmp2 NET=net5 ENGINE_THREADS=1 HASH=64
#   TC=60+0.6 CONC=14 SEED=19 PREPARE=1
#   With ENGINE_THREADS > 1 lower CONC so the games still fit the physical cores (4 threads: CONC=2).
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

ANCHORS="${ANCHORS:-$NNUE_DATA/anchors}"
OURS="${OURS:-$ANCHORS/ours/maharajah-ea3e6a4-nmp2}"
NET="$(net_path "${NET:-net5}")"
# THREADS of env.sh is the training/generation thread count, not the engine's
THREADS="${ENGINE_THREADS:-1}" HASH="${HASH:-64}"
TC="${TC:-60+0.6}" CONC="${CONC:-14}" SEED="${SEED:-19}"
OUT="$ANCHORS/out/tc$TC"

command -v cutechess-cli >/dev/null || die "cutechess-cli not found"
need_file "$OURS"
need_file "$ANCHORS/books/8moves_v3.pgn"
mkdir -p "$OUT"

match() {  # skill anchor games
  local skill=$1 anchor=$2 games=$3 name="s$1_t${THREADS}_vs_$2"
  need_file "$ANCHORS/bin/$anchor"
  echo "== $name, $games games, tc $TC, conc $CONC, $(date)"
  cutechess-cli \
    -engine name="maharajah_s$skill" cmd="$OURS" option.EvalFile="$NET" option.Threads="$THREADS" \
      option.Hash="$HASH" "option.Skill Level=$skill" \
    -engine name="$anchor" cmd="$ANCHORS/bin/$anchor" option.Hash="$HASH" \
    -each proto=uci tc="$TC" timemargin=200 \
    -openings file="$ANCHORS/books/8moves_v3.pgn" format=pgn order=random -repeat \
    -games 2 -rounds $((games / 2)) -concurrency "$CONC" -srand "$SEED" \
    -draw movenumber=40 movecount=8 score=10 -resign movecount=4 score=1000 twosided=true \
    -recover -pgnout "$OUT/$name.pgn" > "$OUT/$name.log" 2>&1 || true
  grep -E '^(Score of|Elo difference)' "$OUT/$name.log" | tail -2
  SEED=$((SEED + 1))
}

l5() {
  match 10 stash-25.0 400
  match 10 stash-27.0 400
}
ladder() {
  match 2 stash-9.0.1 100;  match 2 stash-11.0.1 100
  match 4 stash-11.0.1 100; match 4 stash-12.0 100
  match 6 stash-12.0 100;   match 6 stash-14.0 100
  match 8 stash-14.0 100;   match 8 stash-15.3 100;   match 8 stash-17.0 100
}

(( $# >= 1 )) || die "usage: $0 l5|ladder|all | one SKILL ANCHOR GAMES"
[[ "${PREPARE:-1}" == 1 ]] && prepare_machine
echo "start $(date), engine $OURS, network $NET"
case "$1" in
  l5) l5 ;;
  ladder) ladder ;;
  all) l5; ladder ;;
  one) (( $# == 4 )) || die "usage: $0 one SKILL ANCHOR GAMES"; match "$2" "$3" "$4" ;;
  *) die "unknown mode $1" ;;
esac
echo "done $(date)"
