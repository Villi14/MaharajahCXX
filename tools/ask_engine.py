#!/usr/bin/env python3
"""Ask the Maharajah engine what it would play in a given position.

    python3 tools/ask_engine.py "<fen>" [--level 1-5] [--movetime MS] [--moves "e2e4 e7e5"]

The defaults mirror the app: `--level` is the difficulty shown in the UI (1-5),
and it sets both the engine's skill level and its thinking time exactly as the
app does. So `--level 5` reproduces what difficulty 5 plays.

Build the engine first (see README.md):

    CXXFLAGS= cmake -S . -B build && cmake --build build

Commands are fed with pauses, as for MaharajahC, whose search aborts on any
unread input and then prints `bestmove (none)`; the pauses keep the script
usable against that engine too (`--engine PATH`).
"""

import argparse
import os
import subprocess
import sys
import threading
import time
from pathlib import Path

# sources/EngineConfig.cpp: ui_to_skill_level{ 2, 4, 6, 8, 10 }.
UI_TO_SKILL = {1: 2, 2: 4, 3: 6, 4: 8, 5: 10}
# EnginePolicy.dart: single-player think time is difficulty * 1000 ms.
UI_TO_MOVETIME_MS = {1: 1000, 2: 2000, 3: 3000, 4: 4000, 5: 5000}

START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

# This project builds the engine to build/sources/; MaharajahC to build/, the
# Flutter plugin to src/build/. Windows adds the .exe suffix and MSVC a
# per-config subdirectory.
_BINARY_NAMES = ("Maharajah", "Maharajah.exe")
_BUILD_DIRS = (
    "build/sources",
    "build/sources/Release",
    "build",
    "src/build",
    "build/Release",
    "src/build/Release",
)


def find_engine(explicit=None):
    if explicit:
        path = Path(explicit).expanduser()
        if not path.is_file():
            sys.exit(f"No engine binary at {path}")
        return path

    roots = [Path(__file__).resolve().parent.parent, Path.cwd()]
    roots += list(roots[0].parents)[:2]

    looked, found = [], []
    for root in roots:
        for build_dir in _BUILD_DIRS:
            for name in _BINARY_NAMES:
                candidate = (root / build_dir / name).resolve()
                if candidate.is_file() and os.access(candidate, os.X_OK):
                    found.append(candidate)
                elif candidate not in looked:
                    looked.append(candidate)

    if not found:
        sys.exit(
            "Could not find the engine binary. Build it first:\n"
            "    cmake -S . -B build && cmake --build build\n"
            "or point at it explicitly with --engine PATH.\n"
            "Looked in:\n  " + "\n  ".join(str(p) for p in looked[:8])
        )

    # Several build directories can coexist (an old one next to the current
    # one), and a stale binary would answer for a version of the engine nobody
    # ships. Always take the freshest.
    return max(set(found), key=lambda path: path.stat().st_mtime)


def run(engine, fen, movetime, skill, moves=None, hash_mb=None):
    """Drive one search over UCI and return every line the engine printed."""
    process = subprocess.Popen(
        [str(engine)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        bufsize=1,
    )
    out = []

    def reader():
        for line in process.stdout:
            out.append(line.rstrip())

    threading.Thread(target=reader, daemon=True).start()

    def send(command):
        process.stdin.write(command + "\n")
        process.stdin.flush()
        time.sleep(0.2)

    send("uci")
    if hash_mb:
        send(f"setoption name Hash value {hash_mb}")
    send(f"setoption name Skill Level value {skill}")
    send("isready")
    position = "position startpos" if fen == START_FEN else f"position fen {fen}"
    send(position + (f" moves {moves}" if moves else ""))
    send(f"go movetime {movetime}")

    deadline = time.time() + movetime / 1000 + 10
    while time.time() < deadline:
        if any(line.startswith("bestmove") for line in out):
            break
        time.sleep(0.1)

    send("quit")
    try:
        process.wait(timeout=5)
    except Exception:
        process.kill()
    return out


def describe_move(uci_move):
    """'e7e1' -> 'e7 - e1'; keeps promotions readable as 'e7 - e8 =q'."""
    if not uci_move or len(uci_move) < 4 or uci_move == "(none)":
        return uci_move
    text = f"{uci_move[:2]} - {uci_move[2:4]}"
    if len(uci_move) > 4:
        text += f" ={uci_move[4]}"
    return text


def format_score(tokens):
    """'score cp 34' -> '+0.34'; 'score mate 3' -> 'mate in 3'."""
    try:
        kind = tokens[tokens.index("score") + 1]
        value = int(tokens[tokens.index("score") + 2])
    except (ValueError, IndexError):
        return "?"
    if kind == "mate":
        return f"mate in {abs(value)}" + (" (for the opponent)" if value < 0 else "")
    return f"{value / 100:+.2f}"


def side_to_move(fen):
    fields = fen.split()
    if len(fields) > 1 and fields[1] in ("w", "b"):
        return "White" if fields[1] == "w" else "Black"
    return "?"


def rules_note(fen):
    """Field 7 of the FEN carries per-side variant rights; see Board::parse_fen."""
    fields = fen.split()
    if len(fields) < 7:
        return "derived from the material on the board (no rules field in the FEN)"
    flags = fields[6]
    white = "V" in flags
    black = "v" in flags
    if white and black:
        return "variant for both sides"
    if white:
        return "variant for White, classic for Black"
    if black:
        return "variant for Black, classic for White"
    return "classic for both sides"


def main():
    parser = argparse.ArgumentParser(
        description="Ask the Maharajah engine for its best move in a position.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "examples:\n"
            '  python3 tools/ask_engine.py "8/8/8/8/8/8/6k1/6Q1 w - - 0 1" --level 5\n'
            '  python3 tools/ask_engine.py --startpos --level 3\n'
            '  python3 tools/ask_engine.py "<fen>" --skill 10 --movetime 30000\n'
        ),
    )
    parser.add_argument("fen", nargs="?", help="position in FEN notation")
    parser.add_argument(
        "--startpos", action="store_true", help="use the normal starting position"
    )
    parser.add_argument(
        "--level",
        type=int,
        choices=[1, 2, 3, 4, 5],
        default=5,
        help="difficulty as shown in the app (default: 5)",
    )
    parser.add_argument(
        "--skill", type=int, choices=range(1, 11), metavar="1-10",
        help="raw engine skill level, overrides the one implied by --level",
    )
    parser.add_argument(
        "--movetime", type=int, metavar="MS",
        help="thinking time in milliseconds, overrides the one implied by --level",
    )
    parser.add_argument(
        "--moves", metavar="UCI", help='moves to play from the FEN, e.g. "e2e4 e7e5"'
    )
    parser.add_argument("--hash", type=int, metavar="MB", help="hash table size in MB")
    parser.add_argument(
        "--engine", metavar="PATH",
        default=os.environ.get("MAHARAJAH_ENGINE"),
        help="path to the engine binary (default: auto-detect, or $MAHARAJAH_ENGINE)",
    )
    parser.add_argument(
        "--raw", action="store_true", help="print the raw UCI dialogue instead"
    )
    args = parser.parse_args()

    if args.startpos:
        fen = START_FEN
    elif args.fen:
        fen = args.fen.strip()
    else:
        parser.error("give a FEN, or --startpos")

    if len(fen.split()) < 4:
        parser.error(
            f"that does not look like a FEN (expected at least 4 fields): {fen!r}"
        )

    skill = args.skill if args.skill else UI_TO_SKILL[args.level]
    movetime = args.movetime if args.movetime else UI_TO_MOVETIME_MS[args.level]
    engine = find_engine(args.engine)

    lines = run(engine, fen, movetime, skill, args.moves, args.hash)

    if args.raw:
        print("\n".join(lines))
        return

    print(f"Position : {fen}")
    print(f"To move  : {side_to_move(fen)}")
    print(f"Rules    : {rules_note(fen)}")
    level = "" if args.skill else f"level {args.level}  ->  "
    print(f"Strength : {level}skill {skill}, {movetime} ms of thinking time")
    print(f"Engine   : {engine}")
    print()

    for line in lines:
        if not line.startswith("info"):
            continue
        tokens = line.split()
        try:
            depth = tokens[tokens.index("depth") + 1]
            pv = tokens[tokens.index("pv") + 1:]
        except (ValueError, IndexError):
            continue
        best = pv[0] if pv else "?"
        print(f"  depth {depth:>3}   {format_score(tokens):>10}   {best:<6} {' '.join(pv[1:])}")

    best_moves = [line for line in lines if line.startswith("bestmove")]
    if not best_moves:
        sys.exit("\nThe engine printed no bestmove. Is the binary up to date?")

    best = best_moves[-1].split()[1]
    if best == "(none)":
        print("\nBest move: none")
        print("No legal move — the position is checkmate, stalemate, or malformed.")
    else:
        print(f"\nBest move: {best}   ({describe_move(best)})")


if __name__ == "__main__":
    main()
