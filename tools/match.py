#!/usr/bin/env python3
"""Head-to-head match between two engine builds: a fixed number of pairs, or SPRT.

usage: match.py NEW BASE [--pairs N] [--movetime MS] [--new-movetime MS] [--jobs J] [--seed S] [--custom-share F]
                [--hash MB] [--tc BASE+INC --arbiter TOOL]
                [--sprt ELO0,ELO1 [--alpha A] [--beta B]]
                [--new-option NAME=VALUE ...] [--base-option NAME=VALUE ...]

NEW and BASE are maharajah_tool binaries (go movetime), or with --tc the UCI engine
binaries (go wtime/btime; a path ending in /Maharajah), and then --arbiter names a
maharajah_tool. The Elo interval is 95 %, from the pairs of games of each opening.

Each opening is played twice with colours swapped. Openings: the start position plus
random plies, and generated custom armies plus random plies. Arbiter: a third tool
process on the BASE binary answers status/getfen/legalmoves. --new-option/--base-option
send `setoption` to one engine, e.g. --new-option EvalFile=net.nnue.

--sprt ELO0,ELO1 stops as soon as H0 (NEW is ELO0 stronger) or H1 (ELO1 stronger) is
accepted; logistic Elo, pentanomial GSPRT on the game pairs (the normal approximation
used by fishtest and cutechess). --pairs is then the cap; reaching it is inconclusive.
"""
import argparse, math, random, subprocess, sys, threading, time
from concurrent.futures import ThreadPoolExecutor


class Tool:
    def __init__(self, binary, threads=1, hash_mb=64, options=()):
        # a maharajah_tool takes the `uci` argument; the UCI engine binary (Maharajah) none
        args = [binary] if binary.endswith('/Maharajah') else [binary, 'uci']
        self.p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.send('uci'); self.until('uciok')
        self.send(f'setoption name Hash value {hash_mb}')
        if threads > 1:
            self.send(f'setoption name Threads value {threads}')
        for option in options:
            name, value = option.split('=', 1)
            self.send(f'setoption name {name} value {value}')

    def send(self, line):
        self.p.stdin.write(line + '\n'); self.p.stdin.flush()

    def until(self, prefix):
        lines = []
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError('engine died')
            line = line.strip()
            lines.append(line)
            if line.startswith(prefix):
                return line, lines

    def close(self):
        try:
            self.send('quit'); self.p.wait(timeout=2)
        except Exception:
            self.p.kill()


def position_cmd(fen, moves):
    return f'position fen {fen}' + (' moves ' + ' '.join(moves) if moves else '')


def legal_moves(arb, fen, moves):
    arb.send(position_cmd(fen, moves)); arb.send('legalmoves')
    _, lines = arb.until('endmoves')
    return [l for l in lines[:-1] if l and not l.startswith('error')]


def make_openings(base, pairs, seed, custom_share):
    rng = random.Random(seed)
    arb = Tool(base)
    start = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
    customs = subprocess.run([base, 'generate', str(pairs), str(seed)], capture_output=True, text=True).stdout.split('\n')
    customs = [c.strip() for c in customs if c.strip()]
    openings = []
    while len(openings) < pairs:
        if rng.random() < custom_share and customs:
            fen, plies = customs.pop(), rng.choice([0, 2])
        else:
            fen, plies = start, rng.choice([4, 6, 8])
        moves = []
        for _ in range(plies):
            legal = legal_moves(arb, fen, moves)
            if not legal:
                break
            moves.append(rng.choice(legal))
        arb.send(position_cmd(fen, moves)); arb.send('status')
        if arb.until('status')[0] != 'status ongoing':
            continue
        arb.send('getfen')
        openings.append(arb.until('fen ')[0][4:])
    arb.close()
    return openings


def insufficient(board):
    pieces = [c for c in board if c.isalpha()]
    others = [c for c in pieces if c not in 'Kk']
    return not others or (len(others) == 1 and others[0] in 'NBnb')


def play(new_bin, base_bin, fen, new_white, movetime, threads, max_plies=400, tc=None, arbiter=None, hash_mb=64,
         new_options=(), base_options=(), new_movetime=None):
    """Returns the score of NEW: 1, 0.5 or 0. With tc=(base_ms, inc_ms) the engines play on a clock."""
    new, base = Tool(new_bin, threads, hash_mb, new_options), Tool(base_bin, threads, hash_mb, base_options)
    white, black = (new, base) if new_white else (base, new)
    arb = Tool(arbiter or base_bin)
    clock = [tc[0], tc[0]] if tc else None
    for e in (white, black):
        e.send('ucinewgame')
    side_white = fen.split()[1] == 'w'
    moves, seen = [], {}
    result = 0.5
    try:
        for ply in range(max_plies):
            cmd = position_cmd(fen, moves)
            arb.send(cmd); arb.send('status')
            status = arb.until('status')[0]
            mover_white = side_white == (ply % 2 == 0)
            if status == 'status checkmate':
                result = 0.0 if mover_white else 1.0  # white's score
                break
            if status == 'status stalemate':
                result = 0.5; break
            arb.send('getfen')
            cur = arb.until('fen ')[0][4:].split()
            key = ' '.join(cur[:4])
            seen[key] = seen.get(key, 0) + 1
            if seen[key] >= 3 or insufficient(cur[0]) or (len(cur) > 4 and cur[4].isdigit() and int(cur[4]) >= 100 and not any(c in cur[0] for c in 'AaCcMm')):
                result = 0.5; break
            engine = white if mover_white else black
            engine.send(cmd)
            if tc:
                engine.send('isready'); engine.until('readyok')
                t = time.perf_counter()
                engine.send(f'go wtime {clock[0]} btime {clock[1]} winc {tc[1]} binc {tc[1]}')
                best = engine.until('bestmove')[0].split()[1]
                side = 0 if mover_white else 1
                clock[side] -= int((time.perf_counter() - t) * 1000)
                if clock[side] < 0:
                    result = 0.0 if mover_white else 1.0; break
                clock[side] += tc[1]
            else:
                engine.send(f'go movetime {new_movetime if new_movetime and engine is new else movetime}')
                best = engine.until('bestmove')[0].split()[1]
            if best == '(none)':
                result = 0.0 if mover_white else 1.0; break
            moves.append(best)
        else:
            result = 0.5
    finally:
        for e in (white, black, arb):
            e.close()
    return result if new_white else 1.0 - result


def elo(score):
    score = min(max(score, 1e-6), 1 - 1e-6)
    return -400 * math.log10(1 / score - 1)


def report(pairs_done, label='', sprt=None):
    games = [s for pair in pairs_done for s in pair]
    n = len(games)
    if not n:
        return
    w, d, l = games.count(1.0), games.count(0.5), games.count(0.0)
    mean = sum(games) / n
    # pentanomial: variance from pair sums
    ps = [sum(p) / 2 for p in pairs_done if len(p) == 2]
    if len(ps) > 1:
        m = sum(ps) / len(ps)
        var = sum((x - m) ** 2 for x in ps) / (len(ps) - 1) / len(ps)
        se = math.sqrt(var)
    else:
        se = 0.5 / math.sqrt(n)
    lo, hi = elo(mean - 1.96 * se), elo(mean + 1.96 * se)
    extra = ''
    if sprt:
        llr, lower, upper = sprt_llr(pairs_done, *sprt)
        extra = f'  LLR {llr:+.2f} [{lower:+.2f}, {upper:+.2f}]'
    print(f'{label}games {n}: +{w} ={d} -{l}  {100*mean:.1f}%  {elo(mean):+.0f} Elo [{lo:+.0f}, {hi:+.0f}]{extra}', flush=True)


SPRT_MIN_PAIRS = 20


def sprt_llr(pairs_done, elo0, elo1, alpha, beta):
    """(LLR, lower bound, upper bound) of the pentanomial GSPRT for logistic Elo bounds."""
    lower, upper = math.log(beta / (1 - alpha)), math.log((1 - beta) / alpha)
    ps = [sum(p) / 2 for p in pairs_done if len(p) == 2]
    n = len(ps)
    if n < SPRT_MIN_PAIRS:  # a handful of pairs has a tiny variance and a wild LLR
        return 0.0, lower, upper
    m = sum(ps) / n
    var = sum((x - m) ** 2 for x in ps) / n
    if var <= 0:
        return 0.0, lower, upper
    s0, s1 = (1 / (1 + 10 ** (-e / 400)) for e in (elo0, elo1))
    return n * (s1 - s0) * (2 * m - s0 - s1) / (2 * var), lower, upper


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('new'); ap.add_argument('base')
    ap.add_argument('--pairs', type=int, default=100)
    ap.add_argument('--movetime', type=int, default=100)
    ap.add_argument('--new-movetime', type=int, help='movetime of NEW only (time odds); BASE keeps --movetime')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--threads', type=int, default=1)
    ap.add_argument('--hash', type=int, default=64, help='Hash in MB for both engines')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--custom-share', type=float, default=0.3)
    ap.add_argument('--tc', help='clock games base+inc in seconds, e.g. 5+0.05 (UCI binaries)')
    ap.add_argument('--arbiter', help='maharajah_tool for openings and adjudication (default BASE)')
    ap.add_argument('--sprt', help='ELO0,ELO1: stop when either hypothesis is accepted (--pairs is the cap)')
    ap.add_argument('--alpha', type=float, default=0.05)
    ap.add_argument('--beta', type=float, default=0.05)
    ap.add_argument('--new-option', action='append', default=[], help='NAME=VALUE setoption for NEW (repeatable)')
    ap.add_argument('--base-option', action='append', default=[], help='NAME=VALUE setoption for BASE (repeatable)')
    a = ap.parse_args()
    tc = None
    if a.tc:
        b, i = a.tc.split('+')
        tc = (int(float(b) * 1000), int(float(i) * 1000))
    sprt = None
    if a.sprt:
        e0, e1 = (float(x) for x in a.sprt.split(','))
        sprt = (e0, e1, a.alpha, a.beta)
    openings = make_openings(a.arbiter or a.base, a.pairs, a.seed, a.custom_share)
    results = [[] for _ in openings]
    lock = threading.Lock()
    done = [0]
    stop = threading.Event()
    verdict = ['']
    t0 = time.time()

    def job(i, new_white):
        if stop.is_set():
            return
        s = play(a.new, a.base, openings[i], new_white, a.movetime, a.threads, tc=tc, arbiter=a.arbiter, hash_mb=a.hash,
                 new_options=a.new_option, base_options=a.base_option, new_movetime=a.new_movetime)
        with lock:
            results[i].append(s)
            done[0] += 1
            if done[0] % 50 == 0:
                report([r for r in results if r], f'[{time.time()-t0:.0f}s] ', sprt)
            if sprt and not stop.is_set() and len(results[i]) == 2:
                llr, lower, upper = sprt_llr(results, *sprt)
                if llr <= lower or llr >= upper:
                    verdict[0] = (('H0 accepted (no gain)' if llr <= lower else 'H1 accepted (gain)')
                                  + f' at LLR {llr:+.2f} after {sum(len(r) == 2 for r in results)} pairs')
                    stop.set()

    with ThreadPoolExecutor(a.jobs) as pool:
        futures = [pool.submit(job, i, c) for i in range(len(openings)) for c in (True, False)]
        for f in futures:
            f.result()
    report([r for r in results if r], 'FINAL ', sprt)
    if sprt:
        print(f'SPRT [{sprt[0]:g}, {sprt[1]:g}]: {verdict[0] or "inconclusive, --pairs reached"}'
              ' (games still running at the stop are counted)', flush=True)


if __name__ == '__main__':
    main()
