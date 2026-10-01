#!/usr/bin/env python3
"""Plays identical UCI sessions against MaharajahC and MaharajahCXX and diffs the output."""
import ctypes
import os
import random
import re
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# MaharajahC build (see tools/README.md) and this project's build
C_BUILD = os.environ.get("MAHARAJAH_C_BUILD", os.path.join(REPO, "..", "MaharajahC", "build-compare"))
CXX_BUILD = os.environ.get("MAHARAJAH_CXX_BUILD", os.path.join(REPO, "build"))
LIB_EXT = ".dylib" if sys.platform == "darwin" else ".so"
C_BIN = os.path.join(C_BUILD, "Maharajah")
CXX_BIN = os.path.join(CXX_BUILD, "sources", "Maharajah")
C_LIB = os.path.join(C_BUILD, "libmaharajah_ffi" + LIB_EXT)
CXX_LIB = os.path.join(CXX_BUILD, "sources", "libmaharajah_ffi" + LIB_EXT)

TIME_RE = re.compile(r" time \d+")


class Uci:
    def __init__(self, path):
        self.proc = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self._read_until("uciok")
        self.search_seconds = 0.0

    def _read_until(self, prefix):
        lines = []
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("engine exited")
            line = line.rstrip("\n")
            lines.append(line)
            if line.startswith(prefix):
                return lines

    def send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def search(self, position, go):
        self.send(position)
        start = time.perf_counter()
        self.send(go)
        lines = self._read_until("bestmove")
        self.search_seconds += time.perf_counter() - start
        return [TIME_RE.sub("", line) for line in lines]

    def close(self):
        self.send("quit")
        self.proc.wait(timeout=10)


STANDARD_FENS = [
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1",
    "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9",
    "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40",
    "2b3k1/2p1r1p1/2p2p1p/p1Pp4/8/1PP4P/P1N2PP1/2K1R3 b - - 0 26",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8",
    "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
    "8/5p2/8/2k3P1/p3K3/8/1P6/8 b - - 0 1",
    "6k1/5ppp/8/8/8/8/8/R3K3 w - - 0 1",
    "4k3/8/8/8/8/8/8/4K2R w K - 0 1",
    "8/PPP4k/8/8/8/8/4Kppp/8 w - - 0 1",
    "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 7",
    "3r2k1/pp3pp1/2p4p/8/3P4/2P1R1P1/P4PKP/8 w - - 0 30",
    "8/8/3k4/8/3K4/8/3P4/8 w - - 0 1",
    "r4rk1/pp3ppp/2n1b3/q1pp4/3P4/P1PBPN2/5PPP/R2Q1RK1 w - - 0 14",
    "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1",
    "4k3/4r3/8/8/8/8/3PPP2/3QK3 w - - 0 1",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
]

# compound pieces, variant rules (auto-derived and per-side), unmoved-pawn field
VARIANT_FENS = [
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBMKBNR w - - 0 1",
    "rnamkbcr/pppppppp/8/8/8/8/PPPPPPPP/RNAMKBCR w - - 0 1",
    "4k3/8/8/8/3A4/8/8/4K3 w - - 0 1",
    "6rk/6pp/8/8/2M5/8/8/K7 w - - 0 1",
    "r1c1k2r/ppp2ppp/2n5/3pp3/3PP3/2N5/PPP2PPP/R1A1K2R w - - 0 8",
    "k7/2P5/1K6/8/8/8/8/8 w - - 0 1 V",
    "k7/2P5/1K6/8/8/8/8/8 w - - 0 1 v",
    "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1 v",
    "4k3/8/8/8/8/2P1P3/8/4K3 w - - 0 1 V c3",
    "2m1k3/pppppppp/8/8/8/8/PPPPPPPP/2C1K3 b - - 0 1",
    "4k3/pp3ppp/8/8/8/8/PP3PPP/1M2K1A1 w - - 0 1 Vv",
]

SELFPLAY_OPENINGS = ["", "e2e4", "d2d4", "c2c4", "g1f3", "e2e4 c7c5", "d2d4 g8f6 c2c4 e7e6"]


class Stats:
    def __init__(self):
        self.searches = 0
        self.lines = 0
        self.mismatches = []

    def compare(self, label, c_lines, cxx_lines):
        self.searches += 1
        self.lines += len(c_lines)
        if c_lines != cxx_lines:
            self.mismatches.append((label, c_lines, cxx_lines))
            return False
        return True


def bestmove(lines):
    return lines[-1].split()[1]


def run_positions(c, cxx, stats, fens, depths, tag):
    for fen in fens:
        for depth in depths:
            position = f"position fen {fen}"
            stats.compare(f"{tag} depth {depth}: {fen}", c.search(position, f"go depth {depth}"),
                          cxx.search(position, f"go depth {depth}"))


def selfplay(c, cxx, stats, opening, depth, max_plies, skill=None):
    """Both engines search every position of one game; the (agreed) move is played."""
    moves = opening.split()
    if skill is not None:
        for engine in (c, cxx):
            engine.send(f"setoption name Skill Level value {skill}")
    played = 0
    while played < max_plies:
        position = "position startpos" + (" moves " + " ".join(moves) if moves else "")
        c_lines = c.search(position, f"go depth {depth}")
        cxx_lines = cxx.search(position, f"go depth {depth}")
        label = f"selfplay skill {skill or 10} depth {depth} after [{' '.join(moves)}]"
        if not stats.compare(label, c_lines, cxx_lines):
            return played, "diverged"
        move = bestmove(c_lines)
        if move == "(none)":
            return played, "game over"
        moves.append(move)
        played += 1
    return played, "ply limit"


def fen_selfplay(c, cxx, cxx_lib, stats, fen, depth, max_plies):
    """Self-play from a FEN; each ply is sent as a fresh FEN (avoids 'position fen .. moves')."""
    cxx_lib.mah_set_position_fen(fen.encode())
    buffer = ctypes.create_string_buffer(256)
    played = 0
    while played < max_plies:
        cxx_lib.mah_get_fen(buffer, 256, 1)
        current = buffer.value.decode()
        position = f"position fen {current}"
        c_lines = c.search(position, f"go depth {depth}")
        cxx_lines = cxx.search(position, f"go depth {depth}")
        if not stats.compare(f"custom selfplay depth {depth}: {current}", c_lines, cxx_lines):
            return played, "diverged"
        move = bestmove(c_lines)
        if move == "(none)" or not cxx_lib.mah_apply_move(move.encode()):
            return played, "game over"
        played += 1
    return played, "ply limit"


def ffi_skill_game(c_lib, cxx_lib, stats, skill, depth, plies):
    """Weak skill levels pick among near-best moves with the shared random stream."""
    traces = []
    for lib in (c_lib, cxx_lib):
        lib.mah_init()
        lib.mah_set_skill_level(skill)
        trace = []
        for _ in range(plies):
            move = ctypes.create_string_buffer(8)
            if not lib.mah_best_move_depth(depth, move, 8) or not lib.mah_apply_move(move.value):
                break
            trace.append(move.value.decode())
        lib.mah_set_skill_level(10)
        traces.append(trace)
    stats.compare(f"ffi skill {skill} game", [" ".join(traces[0])], [" ".join(traces[1])])
    return len(traces[1]), "ply limit" if traces[0] == traces[1] else "diverged"


def load_lib(path):
    lib = ctypes.CDLL(path, mode=ctypes.RTLD_LOCAL)
    lib.mah_generate_custom_position_fen.argtypes = [ctypes.c_int, ctypes.c_uint, ctypes.c_char_p, ctypes.c_int]
    lib.mah_get_fen.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
    lib.mah_best_move_depth.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
    lib.mah_init()
    return lib


def compare_ffi(c_lib, cxx_lib, stats, seeds):
    """Custom-position generator, FFI search, game status and FEN after random play."""
    fens = []
    for seed in seeds:
        out = []
        for lib in (c_lib, cxx_lib):
            buffer = ctypes.create_string_buffer(128)
            ok = lib.mah_generate_custom_position_fen(seed % 3, seed, buffer, 128)
            out.append(f"{ok} {buffer.value.decode()}")
        stats.compare(f"ffi generate seed {seed}", [out[0]], [out[1]])
        fens.append(out[1].split(" ", 1)[1])

    rng = random.Random(7)
    for fen in fens[:40]:
        results = []
        for lib in (c_lib, cxx_lib):
            lib.mah_set_position_fen(fen.encode())
            local = random.Random(rng.random())
            trace = []
            for _ in range(30):
                move = ctypes.create_string_buffer(8)
                lib.mah_best_move_depth(3, move, 8)
                status = lib.mah_game_status()
                fen_out = ctypes.create_string_buffer(256)
                lib.mah_get_fen(fen_out, 256, 1)
                trace.append(f"{move.value.decode()} {status} {fen_out.value.decode()}")
                if status != 0:
                    break
                # play a random legal-looking move from the searched one or a pawn push
                if not lib.mah_apply_move(move.value):
                    break
            results.append(trace)
        stats.compare(f"ffi playout from {fen}", results[0], results[1])
    return fens


def main():
    quick = "--quick" in sys.argv
    stats = Stats()
    c, cxx = Uci(C_BIN), Uci(CXX_BIN)
    c_lib, cxx_lib = load_lib(C_LIB), load_lib(CXX_LIB)

    depths = [4, 6] if quick else [5, 7, 8]
    print("standard positions ...", flush=True)
    run_positions(c, cxx, stats, STANDARD_FENS, depths, "standard")
    print("variant positions ...", flush=True)
    run_positions(c, cxx, stats, VARIANT_FENS, depths, "variant")

    print("custom positions (FFI generator) ...", flush=True)
    custom = compare_ffi(c_lib, cxx_lib, stats, range(1, 61 if quick else 201))
    run_positions(c, cxx, stats, custom[:20 if quick else 60], [5] if quick else [6, 7], "custom")

    print("self-play from the start position ...", flush=True)
    games = []
    for opening in SELFPLAY_OPENINGS[:3 if quick else None]:
        games.append(("startpos " + (opening or "-"), 7) + selfplay(c, cxx, stats, opening, 5 if quick else 7, 40 if quick else 120))
    for skill in ([2, 6] if quick else [1, 2, 3, 4, 5, 6, 7, 8, 9]):
        for game in range(2 if quick else 4):
            games.append((f"ffi skill {skill} #{game}", 6) + ffi_skill_game(c_lib, cxx_lib, stats, skill, 6, 40))

    print("self-play from custom positions ...", flush=True)
    for fen in custom[:4 if quick else 12]:
        games.append((fen, 5) + fen_selfplay(c, cxx, cxx_lib, stats, fen, 5, 30 if quick else 80))

    c.close()
    cxx.close()

    print()
    print(f"searches compared : {stats.searches}")
    print(f"info lines compared: {stats.lines}")
    print(f"mismatches        : {len(stats.mismatches)}")
    print(f"search time  C    : {c.search_seconds:.1f}s")
    print(f"search time  C++  : {cxx.search_seconds:.1f}s")
    print()
    for label, depth, plies, end in games:
        print(f"  game {label[:60]:60} plies {plies:3}  ({end})")
    for label, c_lines, cxx_lines in stats.mismatches[:10]:
        print("\nMISMATCH:", label)
        print("  C  :", *c_lines, sep="\n    ")
        print("  C++:", *cxx_lines, sep="\n    ")
    return 1 if stats.mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
