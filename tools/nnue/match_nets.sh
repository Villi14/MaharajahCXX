#!/usr/bin/env bash
# Network against network with the same frozen tool (EvalFile on both sides), one SPRT
# match per opening set: classic (--custom-share 0) and armies (--custom-share 1).
# Logs in $NNUE_DATA/matches/.
#
# Usage: tools/nnue/match_nets.sh NEW BASE [extra match.py arguments]
#   NEW/BASE: .nnue paths or names in NETS_DIR (net6, net5a, net4_768)
#   SHARES="0 1" SPRT=30,50 MOVETIME=50 JOBS=24 PAIRS=400 SEED=71 PREPARE=1
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

(( $# >= 2 )) || die "usage: $0 NEW BASE [match.py arguments]"
NEW="$(net_path "$1")"
BASE="$(net_path "$2")"
shift 2
SHARES="${SHARES:-0 1}" SPRT="${SPRT:-30,50}" MOVETIME="${MOVETIME:-50}"
JOBS="${JOBS:-24}" PAIRS="${PAIRS:-400}" SEED="${SEED:-71}"

need_file "$TOOL"
LOG_DIR="$NNUE_DATA/matches"
mkdir -p "$LOG_DIR"
[[ "${PREPARE:-1}" == 1 ]] && prepare_machine

new_name="$(basename "$NEW" .nnue)" base_name="$(basename "$BASE" .nnue)"
for share in $SHARES; do
  case "$share" in 0) set_name=classic ;; 1) set_name=armies ;; *) set_name="share$share" ;; esac
  log="$LOG_DIR/${new_name}_vs_${base_name}_${set_name}.log"
  echo "== $new_name vs $base_name, $set_name, log $log"
  python3 "$ROOT_DIR/tools/match.py" "$TOOL" "$TOOL" \
    --new-option "EvalFile=$NEW" --base-option "EvalFile=$BASE" \
    --custom-share "$share" --sprt "$SPRT" --pairs "$PAIRS" --movetime "$MOVETIME" \
    --jobs "$JOBS" --seed "$SEED" "$@" 2>&1 | tee "$log"
  SEED=$((SEED + 1))
done
