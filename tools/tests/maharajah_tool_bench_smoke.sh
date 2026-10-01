#!/bin/sh

set -eu

tool_path="$1"

output="$("$tool_path" bench)"

printf '%s\n' "$output" | grep -q '^bench suite positions=5 difficulty=5 mode=classic$'
printf '%s\n' "$output" | grep -q '^bench case=startpos depth=3 move='
printf '%s\n' "$output" | grep -q '^bench case=archbishop_capture depth=2 move=e5f7 '
printf '%s\n' "$output" | grep -q '^bench case=amazon_capture depth=2 move=d5h1 '
printf '%s\n' "$output" | grep -q '^bench summary positions=5 total_nodes='
