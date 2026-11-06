#!/usr/bin/env python3
"""Every set of 1-2 pieces against a bare king, played out at several levels.

usage: bare_king_combos.py ARBITER ENGINE [ENGINE ...] [--per N] [--levels 1,3,5] [--movetime MS] [--jobs J]
ARBITER is a maharajah_tool (rules, status, FEN); each ENGINE a UCI binary, named in
the table by its directory (copy builds to old/Maharajah and new/Maharajah).
The strong side (white, to move) plays at the level, the bare king at level 5.
Orthodox positions get the expected result from the Lichess tablebase; with an
Archbishop (A), Chancellor (C) or Amazon/Maharajah (M) a win is expected.
The tablebase answers are cached in ~/.cache/maharajah_tb_cache.json; the positions
that were not won go to bare_king_failures.txt in the current directory.
Needs python-chess (pip install chess) and network access to tablebase.lichess.ovh.
2026-11-06, 6 positions per set, 100 ms: 1.2.0 level 1 66/241, 1.3.0 level 1 240/241.
"""
import argparse, itertools, json, os, random, sys, time, urllib.request
from concurrent.futures import ThreadPoolExecutor
import chess

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from match import Tool, position_cmd

TYPES = 'QRBNPACM'
LEVEL_TO_SKILL = {1: 2, 2: 4, 3: 6, 4: 8, 5: 10}
CACHE = os.path.expanduser('~/.cache/maharajah_tb_cache.json')
COMPONENTS = {'A': 'BN', 'C': 'RN', 'M': 'QN'}


def attacked_by_white(placement, square):
    """Is `square` attacked by a white piece in placement {sq: symbol}?"""
    for comp_sq, sym in placement.items():
        for part in COMPONENTS.get(sym, sym):
            b = chess.Board(None)
            for s, y in placement.items():  # blockers
                if s != comp_sq:
                    b.set_piece_at(s, chess.Piece.from_symbol('P' if y.isupper() else 'p'))
            b.set_piece_at(comp_sq, chess.Piece.from_symbol(part))
            if b.is_attacked_by(chess.WHITE, square):
                if comp_sq != square:
                    return True
    return False


def random_position(pieces, rng):
    while True:
        squares = rng.sample(range(64), len(pieces) + 2)
        wk, bk, rest = squares[0], squares[1], squares[2:]
        if chess.square_distance(wk, bk) < 2:
            continue
        if any(p == 'P' and chess.square_rank(s) in (0, 7) for p, s in zip(pieces, rest)):
            continue
        # the bare king cannot take anything on its first move
        if any(chess.square_distance(bk, s) < 2 for s in rest):
            continue
        placement = dict(zip(rest, pieces))
        if attacked_by_white(placement, bk):
            continue
        rows = []
        for rank in range(7, -1, -1):
            row, empty = '', 0
            for file in range(8):
                s = chess.square(file, rank)
                sym = 'K' if s == wk else 'k' if s == bk else placement.get(s)
                if sym:
                    row += (str(empty) if empty else '') + sym; empty = 0
                else:
                    empty += 1
            rows.append(row + (str(empty) if empty else ''))
        return '/'.join(rows) + ' w - - 0 1'


def tablebase(fen, cache):
    if fen in cache:
        return cache[fen]
    url = 'https://tablebase.lichess.ovh/standard?fen=' + fen.replace(' ', '_')
    for attempt in range(5):
        try:
            with urllib.request.urlopen(url, timeout=20) as r:
                cat = json.load(r)['category']
            break
        except Exception:
            time.sleep(2 + 3 * attempt)
    else:
        raise RuntimeError('tablebase unreachable')
    cache[fen] = 'win' if cat in ('win', 'cursed-win', 'maybe-win') else 'draw'
    time.sleep(0.6)
    return cache[fen]


def play(arbiter, engine, fen, skill, movetime, max_plies=200):
    strong, weak = Tool(engine, options=[f'Skill Level={skill}']), Tool(engine)
    arb = Tool(arbiter)
    moves, seen = [], {}
    try:
        for ply in range(max_plies):
            cmd = position_cmd(fen, moves)
            arb.send(cmd); arb.send('status')
            status = arb.until('status')[0]
            if status == 'status checkmate':
                return ('win', (ply + 1) // 2) if ply % 2 == 1 else ('LOST', ply)
            if status == 'status stalemate':
                return ('stalemate', ply)
            arb.send('getfen')
            cur = arb.until('fen ')[0][4:].split()
            key = ' '.join(cur[:4]); seen[key] = seen.get(key, 0) + 1
            if seen[key] >= 3:
                return ('repetition', ply)
            if int(cur[4]) >= 100:
                return ('50 moves', ply)
            pieces = [c for c in cur[0] if c.isalpha() and c not in 'Kk']
            if not pieces or (len(pieces) == 1 and pieces[0] in 'NB'):
                return ('insufficient', ply)
            e = strong if ply % 2 == 0 else weak
            e.send(cmd); e.send(f'go movetime {movetime}')
            best = e.until('bestmove')[0].split()[1]
            if best == '(none)':
                return ('none', ply)
            moves.append(best)
        return ('200 plies', max_plies)
    finally:
        for t in (strong, weak, arb):
            t.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('arbiter'); ap.add_argument('engines', nargs='+')
    ap.add_argument('--per', type=int, default=6); ap.add_argument('--levels', default='1,3,5')
    ap.add_argument('--movetime', type=int, default=100); ap.add_argument('--jobs', type=int, default=10)
    ap.add_argument('--seed', type=int, default=11)
    a = ap.parse_args()
    levels = [int(x) for x in a.levels.split(',')]
    combos = [''.join(c) for n in (1, 2) for c in itertools.combinations_with_replacement(TYPES, n)]
    rng = random.Random(a.seed)
    cache = json.load(open(CACHE)) if os.path.exists(CACHE) else {}
    positions = {}
    for c in combos:
        positions[c] = []
        for _ in range(a.per):
            fen = random_position(c, rng)
            orthodox = not any(p in 'ACM' for p in c)
            positions[c].append((fen, tablebase(fen, cache) if orthodox else 'win'))
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    json.dump(cache, open(CACHE, 'w'))

    jobs = [(e, lv, c, fen, exp) for e in a.engines for lv in levels for c in combos for fen, exp in positions[c]]
    with ThreadPoolExecutor(a.jobs) as ex:
        res = list(ex.map(lambda j: play(a.arbiter, j[0], j[3], LEVEL_TO_SKILL[j[1]], a.movetime), jobs))
    table = {}
    for (e, lv, c, fen, exp), (got, n) in zip(jobs, res):
        table.setdefault((c, lv, e), []).append((exp, got, n, fen))

    names = [os.path.basename(os.path.dirname(e)) or e for e in a.engines]
    print(f"wins / tablebase wins per level; {a.per} positions per set, {a.movetime} ms, defender at level 5")
    print('set   TB-won  ' + '  '.join(f'L{lv} {n:>5}' for lv in levels for n in names))
    totals = {}
    for c in combos:
        won = sum(exp == 'win' for _, exp in positions[c])
        cells = []
        for lv in levels:
            for e in a.engines:
                rows = table[(c, lv, e)]
                w = sum(exp == 'win' and got == 'win' for exp, got, _, _ in rows)
                bad = sum(exp == 'draw' and got == 'win' for exp, got, _, _ in rows)
                totals[(lv, e)] = (totals.get((lv, e), (0, 0))[0] + w, totals.get((lv, e), (0, 0))[1] + won)
                cells.append(f'{w}/{won}' + ('!' if bad else '') + ' ' * (8 - len(f'{w}/{won}')))
        print(f'K{c:<4} {won}/{a.per}     ' + '  '.join(cells))
    print('total        ' + '  '.join(f'L{lv} {totals[(lv, e)][0]}/{totals[(lv, e)][1]}' for lv in levels for e in a.engines))
    with open('bare_king_failures.txt', 'w') as f:
        for (c, lv, e), rows in sorted(table.items()):
            for exp, got, n, fen in rows:
                if exp == 'win' and got != 'win':
                    f.write(f'{os.path.basename(os.path.dirname(e))} L{lv} K{c} {got} {fen}\n')


if __name__ == '__main__':
    main()
