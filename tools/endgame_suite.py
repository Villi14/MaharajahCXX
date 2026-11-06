#!/usr/bin/env python3
"""Play classic endgames out with the Maharajah UCI engine on both sides.

usage: endgame_suite.py ENGINE [--movetime MS] [--skill N] [--filter NAME] [-j JOBS]
The side to move plays at --skill, the other side at 10. Each test names the
tablebase result for the side to move; the script prints what happened and the
first score. Single games are noisy: compare two builds over several runs.
Needs python-chess (pip install chess). ENGINE is the UCI binary
(build-release/sources/Maharajah); EvalFile is not set, so the built-in net plays.
"""
import argparse, subprocess, sys, chess
from concurrent.futures import ThreadPoolExecutor

TESTS = [
    # name, fen, expected ("win" = side to move wins, "draw")
    ("Screenshot RNP",  "8/8/3K4/6r1/6k1/4n3/5p2/8 b - - 0 1", "win"),
    ("KR+P v K",       "8/8/8/8/8/2k5/P7/K6R w - - 0 1", "win"),
    ("KQ+N v K",       "8/8/8/4k3/8/8/8/2NQK3 w - - 0 1", "win"),
    ("KBNK-3",         "8/8/8/8/3k4/8/8/4KBN1 w - - 0 1", "win"),
    ("KQK",            "8/8/8/4k3/8/8/8/3QK3 w - - 0 1", "win"),
    ("KRK",            "8/8/8/4k3/8/8/8/R3K3 w - - 0 1", "win"),
    ("KBBK",           "8/8/8/4k3/8/8/8/2B1KB2 w - - 0 1", "win"),
    ("KBNK",           "8/8/8/4k3/8/8/8/2B1KN2 w - - 0 1", "win"),
    ("KBNK-2",         "8/8/3k4/8/8/8/8/1N2K1B1 w - - 0 1", "win"),
    ("KPK 6th rank",   "4k3/8/4K3/4P3/8/8/8/8 w - - 0 1", "win"),
    ("KPK draw",       "8/8/8/3k4/8/3K4/3P4/8 w - - 0 1", "draw"),
    ("Lucena",         "1K1k4/1P6/8/8/8/8/r7/2R5 w - - 0 1", "win"),
    ("KQ v KR",        "8/8/8/3k4/8/8/2r5/3QK3 w - - 0 1", "win"),
    ("KQ v KP(7th)",   "8/8/8/8/8/8/1Kp5/2k1Q3 w - - 0 1", "win"),
    ("KR v KP race",   "8/8/8/8/1k6/8/p7/K3R3 w - - 0 1", "win"),
    ("Q v 3P",         "8/8/8/4K3/8/5k2/4ppp1/7Q w - - 0 1", "win"),
    ("2 connected P",  "8/8/4k3/8/3PP3/4K3/8/8 w - - 0 1", "win"),
    ("outside passer", "8/p4kpp/8/8/8/8/P4KPP/8 w - - 0 1", "draw"),
    ("Wrong bishop",   "7k/8/7K/7P/8/8/8/5B2 w - - 0 1", "draw"),
    ("KR v KB",        "8/8/8/4k3/8/3b4/8/R3K3 w - - 0 1", "draw"),
    ("KNNK",           "8/8/8/4k3/8/8/8/1N2KN2 w - - 0 1", "draw"),
    ("B+2P v B (TB win)", "8/4k3/8/3b4/8/2PB4/1P2K3/8 w - - 0 1", "win"),
    ("Philidor",       "4k3/R7/8/4PK2/8/8/8/7r b - - 0 1", "draw"),
    ("Vancura-ish",    "8/8/R7/P5k1/8/8/6K1/r7 b - - 0 1", "draw"),
]


class Uci:
    def __init__(self, path, skill):
        self.p = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        self.send("uci"); self.wait("uciok")
        self.send(f"setoption name Skill Level value {skill}")
        self.send("isready"); self.wait("readyok")

    def send(self, s): self.p.stdin.write(s + "\n"); self.p.stdin.flush()

    def wait(self, tok):
        lines = []
        while True:
            l = self.p.stdout.readline()
            if not l: raise RuntimeError("engine died")
            lines.append(l.strip())
            if l.startswith(tok): return lines

    def go(self, board, ms):
        root = board.root()
        moves = " ".join(m.uci() for m in board.move_stack)
        self.send(f"position fen {root.fen()}" + (f" moves {moves}" if moves else ""))
        lines = self.send(f"go movetime {ms}") or self.wait("bestmove")
        score = None
        for l in lines:
            t = l.split()
            if "score" in t:
                i = t.index("score"); score = f"{t[i+1]} {t[i+2]}"
        return lines[-1].split()[1], score

    def close(self): self.send("quit"); self.p.wait()


def run(test, path, ms, skill, max_plies):
    name, fen, expected = test
    board = chess.Board(fen)
    attacker = board.turn
    eng = {attacker: Uci(path, skill), not attacker: Uci(path, 10)}
    first = None
    try:
        while not board.is_game_over(claim_draw=True) and board.ply() < max_plies:
            mv, score = eng[board.turn].go(board, ms)
            if first is None: first = score
            board.push_uci(mv)
    finally:
        for e in eng.values(): e.close()
    o = board.outcome(claim_draw=True)
    if o is None: got = f"no result in {max_plies} plies"
    elif o.winner is None: got = f"draw ({o.termination.name.lower()})"
    else: got = ("win" if o.winner == attacker else "LOSS") + f" in {(board.ply()+1)//2} moves"
    ok = got.startswith(expected) or (expected == "win" and got.startswith("win"))
    return name, expected, got, first, ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("engine"); ap.add_argument("--movetime", type=int, default=200)
    ap.add_argument("--skill", type=int, default=10); ap.add_argument("--plies", type=int, default=200)
    ap.add_argument("--filter", default=""); ap.add_argument("-j", type=int, default=6)
    a = ap.parse_args()
    tests = [t for t in TESTS if a.filter.lower() in t[0].lower()]
    with ThreadPoolExecutor(a.j) as ex:
        res = list(ex.map(lambda t: run(t, a.engine, a.movetime, a.skill, a.plies), tests))
    bad = 0
    for name, exp, got, first, ok in res:
        bad += not ok
        print(f"{'ok ' if ok else 'BAD'}  {name:20} want {exp:5}  got {got:32} first score {first}")
    print(f"{len(res)-bad}/{len(res)} correct  (movetime {a.movetime} ms, attacker skill {a.skill})")


if __name__ == "__main__":
    main()
