#!/usr/bin/env python3
"""The kinds of game in NNUE training data, and the gen shares that even them out.

Every line of the data is "FEN;score;result". FEN field 7 tells the kind of game:
  no field 7 / "-"  orthodox against orthodox  (the start position)
  "V" or "v"        orthodox against variant   (gen MIXED_SHARE)
  "Vv"              variant against variant    (gen ANY_SHARE, WIDE_SHARE, CUSTOM_SHARE)

  data_mix.py DATA_GLOB [DATA_GLOB ...]
      positions and games of each kind
  data_mix.py DATA_GLOB [...] --pilot PILOT_GLOB
      also the positions the next round needs so that every kind ends up with as many
      positions as the largest one, and the gen shares (per game) that give them; the
      pilot (a short gen run with MIXED_SHARE and ANY_SHARE both > 0) gives the positions
      per game of each kind under the new round's settings (depth, WIN_SCORE).
      --target N sets the positions per kind instead (e.g. 30e6).

A game starts where the number of men on the board goes up or the kind changes; with
2000 games per chunk this misses ~2 % of the boundaries, which is fine for shares.
"""
import argparse
import glob
import sys
from collections import Counter
from multiprocessing import Pool

KINDS = ("orthodox-orthodox", "orthodox-variant", "variant-variant")
GEN_NAME = {"orthodox-orthodox": "start position (the rest)", "orthodox-variant": "MIXED_SHARE",
            "variant-variant": "ANY_SHARE"}


def kind_of(flag):
    if flag in ("", "-"):
        return KINDS[0]
    return KINDS[2] if flag == "Vv" else KINDS[1]


def count_file(path):
    positions, games = Counter(), Counter()
    previous_men, previous_kind = 99, None
    with open(path, encoding="ascii", errors="replace") as data:
        for line in data:
            fen = line.split(";", 1)[0].split(" ")
            kind = kind_of(fen[6] if len(fen) > 6 else "")
            men = sum(ch.isalpha() for ch in fen[0])
            positions[kind] += 1
            if men > previous_men or kind != previous_kind:
                games[kind] += 1
            previous_men, previous_kind = men, kind
    return positions, games


def count(patterns):
    files = sorted({f for pattern in patterns for f in glob.glob(pattern)})
    if not files:
        sys.exit(f"no files match {' '.join(patterns)}")
    positions, games = Counter(), Counter()
    with Pool() as pool:
        for p, g in pool.imap_unordered(count_file, files):
            positions.update(p)
            games.update(g)
    return files, positions, games


def show(title, files, positions, games):
    total = sum(positions.values())
    print(f"{title}: {len(files)} files, {total:,} positions, {sum(games.values()):,} games")
    for kind in KINDS:
        per_game = positions[kind] / games[kind] if games[kind] else 0
        print(f"  {kind:18s} {positions[kind]:>12,} positions {100 * positions[kind] / total:6.2f} %"
              f"  {games[kind]:>9,} games  {per_game:6.1f} per game")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("data", nargs="+", help="globs of the data the network will train on")
    parser.add_argument("--pilot", help="glob of a pilot run of the next round")
    parser.add_argument("--target", type=float, help="positions per kind after the next round")
    args = parser.parse_args()

    files, positions, games = count(args.data)
    show("data", files, positions, games)
    if not args.pilot:
        return

    pilot_files, pilot_positions, pilot_games = count([args.pilot])
    show("pilot", pilot_files, pilot_positions, pilot_games)
    per_game = {}
    for kind in KINDS:
        if pilot_games[kind] == 0:
            sys.exit(f"the pilot has no {kind} games; run it with MIXED_SHARE and ANY_SHARE > 0 and some start-position games")
        per_game[kind] = pilot_positions[kind] / pilot_games[kind]

    target = args.target or max(positions.values())
    needed = {kind: max(0.0, target - positions[kind]) for kind in KINDS}
    total_needed = sum(needed.values())
    if total_needed == 0:
        print("every kind already has the target")
        return
    game_weight = {kind: needed[kind] / per_game[kind] for kind in KINDS}
    total_weight = sum(game_weight.values())

    print(f"\nnext round: {total_needed:,.0f} positions for {target:,.0f} per kind "
          f"({3 * target:,.0f} in all, a third each)")
    for kind in KINDS:
        print(f"  {kind:18s} {needed[kind]:>12,.0f} positions  game share {game_weight[kind] / total_weight:.3f}"
              f"  ({GEN_NAME[kind]})")
    mixed = game_weight["orthodox-variant"] / total_weight
    variant = game_weight["variant-variant"] / total_weight
    print(f"\n  CUSTOM_SHARE=0 WIDE_SHARE=0 WALL_SHARE=0 MIXED_SHARE={mixed:.3f} ANY_SHARE={variant:.3f}")


if __name__ == "__main__":
    main()
