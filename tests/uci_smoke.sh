#!/usr/bin/env sh
set -eu

engine_path="$1"

# Run a command with a wall-clock bound. Prefer coreutils `timeout`/`gtimeout`,
# but fall back to a POSIX watchdog because stock macOS ships neither, which
# used to make the EOF check below fail spuriously ("command not found").
if command -v timeout >/dev/null 2>&1; then
  timeout_cmd=timeout
elif command -v gtimeout >/dev/null 2>&1; then
  timeout_cmd=gtimeout
else
  timeout_cmd=
fi

run_bounded() {
  secs="$1"
  shift
  if [ -n "$timeout_cmd" ]; then
    "$timeout_cmd" "$secs" "$@"
    return $?
  fi
  "$@" &
  cmd_pid=$!
  ( sleep "$secs"; kill "$cmd_pid" 2>/dev/null ) &
  watcher_pid=$!
  if wait "$cmd_pid" 2>/dev/null; then
    status=0
  else
    status=$?
  fi
  kill "$watcher_pid" 2>/dev/null || true
  wait "$watcher_pid" 2>/dev/null || true
  return "$status"
}
output="$(printf 'uci\nisready\nposition startpos\ngo depth 1\nquit\n' | "$engine_path")"
stalemate_output="$(printf 'uci\nisready\nposition fen k7/2Q5/1K6/8/8/8/8/8 b - - 0 1\ngo depth 1\nquit\n' | "$engine_path")"

printf '%s\n' "$output" | grep -q '^id name Maharajah '
printf '%s\n' "$output" | grep -q '^id author '
printf '%s\n' "$output" | grep -q '^uciok$'
printf '%s\n' "$output" | grep -q '^readyok$'
printf '%s\n' "$output" | grep -q '^bestmove '
printf '%s\n' "$stalemate_output" | grep -q '^bestmove (none)$'

if printf '%s\n' "$output" | grep -q 'Hash table is initial'; then
  echo "uci_smoke failed: unexpected non-UCI log line in stdout" >&2
  exit 1
fi

if printf '%s\n' "$stalemate_output" | grep -q 'Hash table is initial'; then
  echo "uci_smoke failed: unexpected non-UCI log line in stalemate stdout" >&2
  exit 1
fi

# EOF on stdin without an explicit "quit" must terminate the engine (it used
# to busy-loop on the closed pipe at 100% CPU).
if ! printf 'uci\nisready\n' | run_bounded 5 "$engine_path" >/dev/null 2>&1; then
  echo "uci_smoke failed: engine did not exit on stdin EOF" >&2
  exit 1
fi

if printf '%s\n' "$output" | grep -q '^bestmove a8a8$'; then
  echo "uci_smoke failed: invalid null bestmove from non-terminal position" >&2
  exit 1
fi
