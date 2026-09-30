#!/bin/sh
set -eu

tool_path="$1"

output="$(printf '%s\n' \
  'uci' \
  'isready' \
  'position startpos moves e2e4' \
  'position startpos moves e2e4 e7e5' \
  'getfen 2' \
  'status' \
  'go depth 3' \
  'position startpos moves e2e4 zzzz' \
  'position fen k7/2Q5/1K6/8/8/8/8/8 b - - 0 1' \
  'status' \
  'legalmoves' \
  'position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 moves a1a8' \
  'status' \
  'quit' | "$tool_path" uci 2>/dev/null)"

expect() {
  if ! printf '%s\n' "$output" | grep -qx -- "$1"; then
    echo "maharajah_tool_uci_smoke failed: missing line '$1'" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

expect 'id name Maharajah .* (tool)'
expect 'uciok'
expect 'readyok'
expect 'fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2 -'
expect 'status ongoing'
expect 'bestmove [a-h][1-8][a-h][1-8][qrbnacm]*'
expect 'error position'
expect 'status stalemate'
expect 'endmoves'
expect 'status checkmate'

# the stalemated side has no legal moves: "endmoves" directly follows "status stalemate"
printf '%s\n' "$output" | grep -A1 -x 'status stalemate' | grep -qx 'endmoves'

moves="$("$tool_path" legalmoves 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')"
test "$(printf '%s\n' "$moves" | wc -l | tr -d ' ')" = 20
printf '%s\n' "$moves" | grep -qx 'e2e4'
