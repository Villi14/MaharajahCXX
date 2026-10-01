#!/usr/bin/env python3
"""Compare perft counts of the UCI engine (`go perft N`) with Fairy-Stockfish.

usage: compare_perft.py ENGINE FSF VARIANTS_INI [--depth D] [--generated N] [--seed S]
                        [--tool maharajah_tool]

ENGINE is the UCI binary (build-release/sources/Maharajah), FSF a Fairy-Stockfish built
with largeboards=yes, VARIANTS_INI the Maharajah_lab `fsf/variants.ini` (variant
`maharajah`: chess plus archbishop A, chancellor C, maharajah M, promotion to them).

Only positions FSF can represent are compared: both sides classic (FSF `chess`) or
both variant (field 7 `Vv`, FSF `maharajah`) without castling rights, where exactly the
pawns on the home rank are unmoved (FSF double-steps from the home rank only).
With --generated N, custom armies from `maharajah_tool generate` are added and the
ones outside that set are skipped. On a mismatch the per-move counts are compared.
"""
import argparse, subprocess, sys

POSITIONS = [
    # orthodox, FSF `chess`
    'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
    'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1',
    '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1',
    # both sides variant, FSF `maharajah`
    '7k/8/8/3M4/8/8/8/K7 w - - 0 1 Vv -',
    '7k/8/8/3A4/4C3/8/8/4K3 w - - 0 1 Vv -',
    '4k3/8/8/3m4/8/8/8/K7 b - - 0 1 Vv -',
    '4k3/pppppppp/8/8/3M4/8/PPPPPPPP/4K3 w - - 0 1 Vv a2b2c2d2e2f2g2h2a7b7c7d7e7f7g7h7',
    'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBMKBNR w - - 0 1 Vv a2b2c2d2e2f2g2h2a7b7c7d7e7f7g7h7',
    'cnbakbnm/pppppppp/8/8/8/8/PPPPPPPP/CNBAKBNM w - - 0 1 Vv a2b2c2d2e2f2g2h2a7b7c7d7e7f7g7h7',
    # promotions to the compound pieces, also with a capture
    '1r2k3/P7/8/8/8/8/6p1/4K2R w - - 0 1 Vv -',
    '4k3/1P4P1/8/2m5/5A2/8/1p4p1/4K3 b - - 0 1 Vv -',
    # en passant next to compound pieces
    '4k3/8/8/2PpP3/8/8/8/M3K2c w - d6 0 1 Vv -',
    # checks and pins by the knight part of a compound piece
    '4k3/8/3N4/8/1a6/8/3P4/c3K3 w - - 0 1 Vv d2',
    'm3k3/8/8/8/8/8/2PPP3/1N2K2C w - - 0 1 Vv c2d2e2',
]


class Uci:
    def __init__(self, args, setup=()):
        self.p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.send('uci'); self.until('uciok')
        for line in setup:
            self.send(line)
        self.send('isready'); self.until('readyok')

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
                return lines

    def perft(self, fen, depth):
        self.send(f'position fen {fen}'); self.send(f'go perft {depth}')
        lines = self.until('Nodes searched')
        moves = {}
        for line in lines[:-1]:
            move, sep, count = line.partition(': ')
            if sep and count.isdigit():
                moves[move] = int(count)
        return int(lines[-1].split()[-1]), moves

    def close(self):
        self.send('quit'); self.p.wait(timeout=5)


def fsf_variant(fen):
    """FSF variant name for a Maharajah FEN, or None when FSF cannot represent it."""
    fields = fen.split()
    rights = fields[6] if len(fields) > 6 else '-'
    if rights == '-':
        return None if any(c in fields[0] for c in 'AaCcMm') else 'chess'
    # without field 8 every pawn on its own half counts as unmoved
    if rights != 'Vv' or fields[2] != '-' or len(fields) < 8:
        return None
    # exactly the home-rank pawns unmoved, as FSF assumes
    rows = []
    for row in fields[0].split('/'):
        expanded = ''
        for c in row:
            expanded += '.' * int(c) if c.isdigit() else c
        rows.append(expanded)
    home = {f'{"abcdefgh"[f]}2' for f in range(8) if rows[6][f] == 'P'} | \
           {f'{"abcdefgh"[f]}7' for f in range(8) if rows[1][f] == 'p'}
    field8 = fields[7]
    unmoved = set() if field8 == '-' else {field8[i:i + 2] for i in range(0, len(field8), 2)}
    return 'maharajah' if unmoved == home else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine'); ap.add_argument('fsf'); ap.add_argument('variants_ini')
    ap.add_argument('--depth', type=int, default=4)
    ap.add_argument('--generated', type=int, default=0)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--tool', help='maharajah_tool for --generated')
    a = ap.parse_args()

    fens = list(POSITIONS)
    if a.generated:
        out = subprocess.run([a.tool, 'generate', str(a.generated), str(a.seed)], capture_output=True, text=True).stdout
        fens += [l.strip() for l in out.splitlines() if l.strip()]

    ours = Uci([a.engine])
    fsf = {v: Uci([a.fsf], [f'setoption name VariantPath value {a.variants_ini}', f'setoption name UCI_Variant value {v}'])
           for v in ('chess', 'maharajah')}
    compared = skipped = mismatches = 0
    for fen in fens:
        variant = fsf_variant(fen)
        if not variant:
            skipped += 1
            continue
        fsf_fen = ' '.join(fen.split()[:6])
        for depth in range(1, a.depth + 1):
            n_ours, m_ours = ours.perft(fen, depth)
            n_fsf, m_fsf = fsf[variant].perft(fsf_fen, depth)
            compared += 1
            if n_ours != n_fsf:
                mismatches += 1
                print(f'MISMATCH {fen} depth {depth}: ours {n_ours}, fsf {n_fsf}')
                for move in sorted(set(m_ours) | set(m_fsf)):
                    if m_ours.get(move) != m_fsf.get(move):
                        print(f'  {move}: ours {m_ours.get(move)}, fsf {m_fsf.get(move)}')
                break
        else:
            print(f'ok {variant:9} d{a.depth} {n_ours:>10}  {fen}')
    for e in (ours, *fsf.values()):
        e.close()
    print(f'{compared} perft counts compared, {mismatches} mismatches, {skipped} positions skipped')
    sys.exit(1 if mismatches else 0)


if __name__ == '__main__':
    main()
