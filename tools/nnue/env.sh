# Shared settings of the NNUE round scripts (gen.sh, train.sh, match_nets.sh, check.sh).
# Sourced, not run. Every value can be overridden from the environment.

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
NNUE_DATA="${NNUE_DATA:-$HOME/nnue-data}"
BIN_DIR="${BIN_DIR:-$ROOT_DIR/build-release/tools}"
TEXEL="${TEXEL:-$BIN_DIR/maharajah_texel}"
TRAIN="${TRAIN:-$BIN_DIR/maharajah_nnue_train}"
# frozen e07a949 tool; the network under test is loaded with EvalFile
TOOL="${TOOL:-$NNUE_DATA/m101/bin/tool_base}"
NETS_DIR="${NETS_DIR:-$NNUE_DATA/nets}"
THREADS="${THREADS:-24}"

die() { echo "error: $*" >&2; exit 1; }

need_file() { [[ -f "$1" ]] || die "missing $1"; }

# a .nnue argument: a path, or a name in NETS_DIR (net5a, net5a.nnue)
net_path() {
  local net="$1"
  [[ -f "$net" ]] && { echo "$net"; return; }
  [[ "$net" == *.nnue ]] || net="$net.nnue"
  [[ -f "$NETS_DIR/$net" ]] && { echo "$NETS_DIR/$net"; return; }
  [[ -f "$ROOT_DIR/nets/$net" ]] && { echo "$ROOT_DIR/nets/$net"; return; }
  die "network $1 not found (NETS_DIR=$NETS_DIR)"
}

# The machine state every match and training run assumes (TODO.md): background generation
# stopped, fans up, turbo off. Asks for sudo once.
prepare_machine() {
  if systemctl is-active --quiet nnue-gen 2>/dev/null; then
    echo "stopping nnue-gen"
    sudo systemctl stop nnue-gen
  fi
  if [[ -x "$NNUE_DATA/r4/fan.sh" ]]; then
    sudo "$NNUE_DATA/r4/fan.sh" 90
  else
    echo "warning: $NNUE_DATA/r4/fan.sh not found, fans left as they are" >&2
  fi
  local turbo=/sys/devices/system/cpu/intel_pstate/no_turbo
  if [[ -f "$turbo" && "$(cat "$turbo")" != 1 ]]; then
    echo "turning turbo off"
    echo 1 | sudo tee "$turbo" >/dev/null
  fi
  [[ ! -f "$turbo" ]] || echo "no_turbo = $(cat "$turbo")"
}

# Release build check (a Debug binary plays ~300 Elo weaker and generates far slower)
check_release() {
  local cache="$BIN_DIR/../CMakeCache.txt"
  if [[ -f "$cache" ]]; then
    local type
    type="$(sed -n 's/^CMAKE_BUILD_TYPE:STRING=//p' "$cache")"
    [[ "$type" == Release ]] || die "$BIN_DIR is a '$type' build, Release needed"
  fi
}
