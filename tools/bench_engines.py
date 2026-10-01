#!/usr/bin/env python3
"""Search speed of MaharajahC vs MaharajahCXX: same positions, same depth, same node counts."""
import os
import statistics
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# MaharajahC build (see tools/README.md) and this project's build
C_BUILD = os.environ.get("MAHARAJAH_C_BUILD", os.path.join(REPO, "..", "MaharajahC", "build-compare"))
CXX_BUILD = os.environ.get("MAHARAJAH_CXX_BUILD", os.path.join(REPO, "build"))
LIB_EXT = ".dylib" if sys.platform == "darwin" else ".so"
ENGINES = {"C": os.path.join(C_BUILD, "Maharajah"), "C++": os.path.join(CXX_BUILD, "sources", "Maharajah")}

# (name, fen, depth) chosen so each search takes roughly 0.2-1.5 s on an Apple M2 Pro
SUITE = [
    ("start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 12),
    ("kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 9),
    ("middlegame", "r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8", 10),
    ("cmk", "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9", 10),
    ("endgame", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 13),
    ("rook ending", "3r2k1/pp3pp1/2p4p/8/3P4/2P1R1P1/P4PKP/8 w - - 0 30", 13),
    ("compound", "rnamkbcr/pppppppp/8/8/8/8/PPPPPPPP/RNAMKBCR w - - 0 1", 10),
    ("custom army", "bbmbkq1c/8/8/8/8/8/4P3/RAMNKBBR w - - 0 1 Vv", 10),
]


def run(path, fen, depth):
    """Returns (seconds, nodes, bestmove) for one fresh-process search."""
    proc = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=1)

    def read_until(prefix):
        lines = []
        while True:
            line = proc.stdout.readline()
            lines.append(line)
            if line.startswith(prefix):
                return lines

    read_until("uciok")
    proc.stdin.write(f"position fen {fen}\nisready\n")
    proc.stdin.flush()
    read_until("readyok")
    start = time.perf_counter()
    proc.stdin.write(f"go depth {depth}\n")
    proc.stdin.flush()
    lines = read_until("bestmove")
    elapsed = time.perf_counter() - start
    proc.stdin.write("quit\n")
    proc.stdin.flush()
    proc.wait()
    info = [line for line in lines if line.startswith("info")][-1].split()
    return elapsed, int(info[info.index("nodes") + 1]), lines[-1].split()[1]


def main():
    repeats = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    totals = {name: [0.0, 0] for name in ENGINES}
    ratios = []
    print(f"{'position':12} {'depth':>5} {'nodes':>10} {'C ms':>8} {'C++ ms':>8} {'C kN/s':>8} {'C++ kN/s':>9} {'C++/C':>6}")
    for label, fen, depth in SUITE:
        times = {name: [] for name in ENGINES}
        results = {}
        for _ in range(repeats):
            for name, path in ENGINES.items():  # alternate to spread thermal / background noise
                elapsed, nodes, move = run(path, fen, depth)
                times[name].append(elapsed)
                results[name] = (nodes, move)
        assert results["C"] == results["C++"], (label, results)
        nodes = results["C"][0]
        median = {name: statistics.median(values) for name, values in times.items()}
        ratios.append(median["C"] / median["C++"])
        for name in ENGINES:
            totals[name][0] += median[name]
            totals[name][1] += nodes
        print(f"{label:12} {depth:5} {nodes:10} {median['C'] * 1000:8.0f} {median['C++'] * 1000:8.0f} "
              f"{nodes / median['C'] / 1000:8.0f} {nodes / median['C++'] / 1000:9.0f} {median['C'] / median['C++']:6.2f}")
    c_time, nodes = totals["C"]
    cxx_time = totals["C++"][0]
    print(f"{'total':12} {'':5} {nodes:10} {c_time * 1000:8.0f} {cxx_time * 1000:8.0f} "
          f"{nodes / c_time / 1000:8.0f} {nodes / cxx_time / 1000:9.0f} {c_time / cxx_time:6.2f}")
    print(f"geometric mean of per-position speed ratios (C++/C): {statistics.geometric_mean(ratios):.2f}")


if __name__ == "__main__":
    main()
