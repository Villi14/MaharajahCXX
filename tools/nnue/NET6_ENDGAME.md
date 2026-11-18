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
2. **Data round r6: endgames and the missing armies** (2026-11-18, after H. G. Muller's
   critique on chessvariants.com). The old rounds started custom games only from
   `generate_custom_position`: 5-8 pawns, a compound piece, few repeated pieces, and never
   the orthodox army against a variant one. NNUE scored the 27P+4N swarm 6-0 against the
   orthodox army where the classic evaluation scored it 1-5. r6 adds only what was missing,
   on top of the existing data (target 8-12 M positions, 15-25 % of the training set):
   ```sh
   ROUND=r6 NET=net5 WIN_SCORE=0 CUSTOM_SHARE=0 WIDE_SHARE=0.35 MIXED_SHARE=0.45 \
     tools/nnue/gen.sh 48                                  # in tmux; sudo once
   ```
   - `MIXED_SHARE`: the orthodox army (castling) against a variant army.
   - `WIDE_SHARE`: variant armies of any shape: 0-27 pawns, one favourite piece type
     (knight swarms, several queens), compound piece optional.
   - the remaining 20 % start from the start position: the orthodox endgames.
   `generate_training_position` in CustomSetup draws these; players' armies are unchanged.
   `DEPTH` (default 7) stays as in round 5 unless the pilot says otherwise. Give the pilot
   the same shares, since swarm games may run longer.
3. **Train net6** on the old rounds plus r6. The trainer's limit is 115 M positions, and
   `train.sh` prints the total, memory and time first:
   ```sh
   tools/nnue/train.sh net6                 # n3d7 + r4 + r5 + r6 (R6_GLOB) by default
   ```
   Same recipe as net5 (mirror, λ 0.75, 20 epochs). If over the limit, drop n3d7 first:
   `N3D7_GLOB=/dev/null/none`.
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
