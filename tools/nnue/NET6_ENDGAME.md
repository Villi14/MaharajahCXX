# net6: a network that knows endgames and every kind of army

Plan of 2026-11-06, after engine 1.3.0; the armies added 2026-11-18 (step 2). Run on the
training machine (Ubuntu, `~/nnue-data`).

**Optional.** Engine 1.3.0 already fixes what players saw: mates against a bare king on every
level (the level 3 game that shuffled past move 249) and the simplest dead draws. net6 is
the next step for the rest of the endgame, when the training machine is free. Nothing
waits on it.

## Why

net5's data comes from `maharajah_texel gen`, which stops a game once a side has scored
≥ +1000 for 6 plies and labels it won. So almost no game reaches a converted endgame, and
the network is weak exactly there:

- Won positions are out of distribution. With rook + knight + pawn against a bare king, net5
  scored a king move (+33.0) above promoting (+29.2); level 3 shuffled past move 249.
- Dead draws scored as wins: KPK draw +9.5, wrong bishop +7.6, KNNK +4.7, KR v KB +3.3. The
  engine trades into "won" endgames that are draws.
- Philidor defence still lost at level 5.

Engine 1.3.0 (`headers/Endgame.h`) fixes the bare-king cases and the simplest dead draws in
code. Everything else, such as rook endings, pawn endings with opposition and whether to
trade into an ending, has to come from the network, which means from data with
endgames played out.

## What changed for this round

- `maharajah_texel gen ... [win_score]`, `gen.sh` `WIN_SCORE`: `0` turns win adjudication
  off. Games run to mate, a draw rule (repetition, 50 moves, insufficient material, the
  ±15 draw adjudication from ply 80) or 400 plies. Default 1000 = old behaviour.
- gen never writes bare-king positions with mating pieces: Endgame.h scores them, not the
  net.
- The teacher is 1.3.0 (Endgame.h is applied in the search), so drawn endings get scores
  near 0 instead of +5. The labels are better than net5's own.
- Measured on the Mac, 40 games, depth 6, net5: WIN_SCORE=1000 gives 3062 positions,
  19 % with ≤ 8 pieces. WIN_SCORE=0 gives 4175 positions, 27 % with ≤ 8 pieces (1112 vs 591).

## Steps

```sh
cd ~/MaharajahCXX && git pull            # main with the gen WIN_SCORE / WIDE_SHARE / MIXED_SHARE options
cmake --build build-release -j           # Release; check.sh shows the build type
pip install --user chess                 # for the endgame checks
tools/nnue/check.sh                      # binaries, nets, data, memory, disk, turbo
```

1. **Pilot, 1 hour.** Measure positions per hour without adjudication. Earlier rounds did
   ~0.36 M/h at depth 8 on 24 threads.
   ```sh
   ROUND=r6p NET=net5 WIN_SCORE=0 tools/nnue/gen.sh 1
   ```
   If the rate collapses (long shuffling games), use `WIN_SCORE=3000` as the middle ground:
   still past net5's range, much shorter games.
2. **Data round r6: every kind of game and every army, evened out** (2026-11-18/19,
   after H. G. Muller's critique on chessvariants.com). net5's 40.25 M positions (r4,
   `tools/nnue/data_mix.py`, 2026-11-19): orthodox against orthodox 55.6 %, variant
   against variant 44.4 %, **orthodox against variant 0**. The variant armies all came
   from `generate_custom_position`: 277 of the 2580 possible armies, the 50 commonest
   87 % of them; 5-8 pawns in 86 %, one maharajah in 52 %, never 16+ pawns, never two
   maharajahs, 5+ knights in 0.03 %. NNUE scored the 27P+4N swarm 6-0 against the
   orthodox army where the classic evaluation scored it 1-5.

   Decided 2026-11-19 (owner): keep only r4's orthodox games, drop its variant ones
   (their 277 armies would outweigh the balanced ones), and generate both other kinds
   anew. r4's orthodox positions, 22,376,330, are in `$NNUE_DATA/r4o` (r4 itself kept):
   ```sh
   mkdir "$NNUE_DATA/r4o" && for f in "$NNUE_DATA"/r4/chunk_*.txt; do
     awk -F';' '{ n = split($1, a, " "); if(n < 7 || a[7] == "-") print }' "$f" > "$NNUE_DATA/r4o/${f##*/}"
   done
   ```
   r6 then needs ~22.4 M orthodox against variant and ~22.4 M variant against variant,
   ~45 M positions, no start-position games; the three kinds a third each.
   - `MIXED_SHARE`: the orthodox army (castling) against a balanced variant army.
   - `ANY_SHARE`: two balanced variant armies, or one and its mirror image (a quarter;
     equal armies teach that equal is equal).
   - Balanced variant army: any of the 2580 armies a player can build (39 points, at
     most 31 men; 313 pawnless; up to 13 knights or bishops, 7 rooks, 6 archbishops,
     4 chancellors, 4 queens, 3 maharajahs) half the time — half of that evenly, half a
     type and its number first, so 3 maharajahs or 4 queens are as common as the middle
     of the range; all 2580 came up in 100000 starts, the rarest 22 times. A pawn wall
     (1-3 solid ranks, 8-27 pawns) with a swarm behind it (knights 35 %, bishops 35 %,
     both 15 %, rooks 10 %, queens 5 %) a fifth; a wide army or a players' army the rest.
   - Every variant army is placed by the players' rule (game_rules.md §6: ranks filled
     from the home rank outward), in a random order: pieces behind the pawns, all mixed,
     or pawns on the home rank with the pieces in front.
   - `WIDE_SHARE` and `WALL_SHARE` still exist but are not used: wall starts mix kinds
     (a third against the orthodox army), so the shares would not set the mix exactly.
     Their armies are part of the balanced variant army.

   Pilot first, with all three kinds, to measure positions per game of each kind:
   ```sh
   ROUND=r6p NET=net5 WIN_SCORE=0 CUSTOM_SHARE=0 MIXED_SHARE=0.4 ANY_SHARE=0.4 tools/nnue/gen.sh 1
   python3 tools/nnue/data_mix.py "$NNUE_DATA/r4o/chunk_*.txt" --pilot "$NNUE_DATA/r6p/chunk_*.txt"
   ```
   `data_mix.py` prints the positions r6 needs and the shares per game (with equal
   positions per game: MIXED_SHARE = ANY_SHARE = 0.5, no start-position games).
   If the rate collapses (long shuffling games), use `WIN_SCORE=3000` as the middle
   ground. A chunk ends with its longest game: in the pilot's 2000-game chunks the last
   minute of six ran one game on one thread, so the main run takes 20000-game chunks
   (pilot 2026-11-19: 1.66 M positions/h, 71 searches cut at 1 s per chunk). Then, in tmux (sudo once), until data_mix.py shows the three kinds even:
   ```sh
   ROUND=r6 NET=net5 WIN_SCORE=0 CUSTOM_SHARE=0 MIXED_SHARE=<printed> ANY_SHARE=<printed> \
     GAMES_PER_CHUNK=20000 tools/nnue/gen.sh 48
   python3 tools/nnue/data_mix.py "$NNUE_DATA/r4o/chunk_*.txt" "$NNUE_DATA/r6/chunk_*.txt"
   ```
   `DEPTH` (default 7) stays as in round 5 unless the pilot says otherwise.
3. **Train net6** on the old rounds plus r6. The trainer's limit is 115 M positions, and
   `train.sh` prints the total, memory and time first:
   ```sh
   N3D7_GLOB=/dev/null/none R4_GLOB="$NNUE_DATA/r4o/chunk_*.txt" tools/nnue/train.sh net6   # r4o + r6
   ```
   Same recipe as net5 (mirror, λ 0.75, 20 epochs), ~67 M positions (limit 115 M).
   n3d7 or r4's variant games would undo the even mix. Check net6 against net5 in the
   classic match (step 5) before anything else: it lost r4's 17.9 M variant positions
   and gained 22.4 M orthodox-against-variant ones, so classic strength must hold.
4. **Armies check.** `Maharajah_ffi/tool/army_match.py` (monorepo) on the pairs from the
   Muller thread, net5 vs net6 and each against `--classic-eval`; the wall vs orthodox at
   levels 1-5, 10 games per colour (net5 2026-11-17: wall 68-29). net6 is right if it moves
   towards the classic evaluation where the two disagreed, not if the orthodox side wins.
   If one round leaves much of net5's error (λ 0.75 = three quarters of the target is the
   teacher's score), a small r7 of mixed games with net6 as the teacher is the next step.
5. **Match net6 vs net5.** The default SPRT 30,50 expects a big jump. For an endgame net, ask
   only "not worse":
   ```sh
   SPRT=-5,5 tools/nnue/match_nets.sh net6 net5
   ```
6. **Endgame checks** with the UCI engine on each net (copy the binary into two
   directories, set the net with `EvalFile` or embed it with `tools/embed_net.py`):
   ```sh
   python3 tools/endgame_suite.py build-release/sources/Maharajah --skill 10 -j 12
   python3 tools/bare_king_combos.py build-release/tools/maharajah_tool old/Maharajah new/Maharajah --per 6
   ```
   What to look for: the KPK draw, Philidor, Vancura and the outside-passer test drawn; the
   first score in dead draws near 0 *without* Endgame.h's help (the suite prints it).
7. **Ship.** `python3 tools/embed_net.py nets/net6.nnue`, bump the version (CMakeLists here,
   and in the monorepo `Maharajah_ffi/src/CMakeLists.txt`, `Maharajah_ffi/pubspec.yaml`,
   `Maharajah_ffi/CHANGELOG.md`). Copy the sources into `Maharajah/Maharajah_ffi/src`.
   For the site: `bash Maharajah_ffi/tool/build_wasm.sh`, then one new site-wide `?v=` in
   `Maharajah_net/Maharajah.Api/wwwroot`, then `tool/play_e2e.sh`, `tool/play_phone_e2e.sh`
   and `tool/assist_e2e.sh`. Redo STRENGTH.md with `tools/nnue/ccrl_match.sh all`.

## Open / not done

- Engine 1.3.0 level 5 vs 1.2.0: SPRT stopped at 650 games, −14 Elo [−37, +8], inconclusive.
  That build evaluated 4–5 % slower (Endgame::adjust counted material on every evaluation).
  The fast exit for positions with pawns brought it to ~1 %, with identical node counts.
  Re-run on this machine with the current build:
  `python3 tools/match.py new/Maharajah old/Maharajah --tc 5+0.05 --arbiter TOOL --sprt=-10,0 --pairs 1200 --jobs 24`.
- Syzygy tablebases (3–5 pieces) for gen labels would be the next step up. They are not in the
  engine, and the app should not need them.
- Draw adjudication (±15 from ply 80 for 12 plies) is kept; it trusts the teacher in drawn
  endings, which 1.3.0 improves only for the cases in Endgame.h.
