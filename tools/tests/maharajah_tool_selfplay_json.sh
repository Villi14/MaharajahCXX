#!/usr/bin/env sh
set -eu

tool_path="$1"
json_path="$2"

rm -f "$json_path"
"$tool_path" selfplay 1 1 20 7 "$json_path" >/dev/null

test -f "$json_path"
grep -q '"game": 1' "$json_path"
grep -q '"start_fen": "' "$json_path"
grep -q '"result": "' "$json_path"
grep -q '"end_fen": "' "$json_path"
grep -q '"moves": \[' "$json_path"

if ! grep -Eq '"result": "(white_checkmate|black_checkmate|stalemate|move_limit|ongoing)"' "$json_path"; then
  echo "maharajah_tool_selfplay_json failed: result field missing or invalid." >&2
  exit 1
fi
