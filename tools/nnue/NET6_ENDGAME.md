# net6: a network that knows endgames

Plan of 2026-11-06, after engine 1.3.0. Run on the training machine (Ubuntu, `~/nnue-data`).

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
cd ~/MaharajahCXX && git pull            # main ≥ 24b4ac3 + the gen change
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
2. **Data round r6**, as many hours as the machine can spare (target ≥ 20 M positions):
   ```sh
   ROUND=r6 NET=net5 WIN_SCORE=0 tools/nnue/gen.sh 48     # in tmux; sudo once
   ```
   `CUSTOM_SHARE` (default 0.65) and `DEPTH` (default 7) stay as in round 5 unless the
   pilot says otherwise.
3. **Train net6** on the old rounds plus r6. The trainer's limit is 115 M positions, and
   `train.sh` prints the total, memory and time first:
   ```sh
   R5_GLOB="$HOME/nnue-data/r5/chunk_*.txt $HOME/nnue-data/r6/chunk_*.txt" tools/nnue/train.sh net6
   ```
   Same recipe as net5 (mirror, λ 0.75, 20 epochs). If over the limit, drop n3d7 first:
   `N3D7_GLOB=/dev/null/none`.
4. **Match net6 vs net5.** The default SPRT 30,50 expects a big jump. For an endgame net, ask
   only "not worse":
   ```sh
   SPRT=-5,5 tools/nnue/match_nets.sh net6 net5
   ```
5. **Endgame checks** with the UCI engine on each net (copy the binary into two
   directories, set the net with `EvalFile` or embed it with `tools/embed_net.py`):
   ```sh
   python3 tools/endgame_suite.py build-release/sources/Maharajah --skill 10 -j 12
   python3 tools/bare_king_combos.py build-release/tools/maharajah_tool old/Maharajah new/Maharajah --per 6
   ```
   What to look for: the KPK draw, Philidor, Vancura and the outside-passer test drawn; the
   first score in dead draws near 0 *without* Endgame.h's help (the suite prints it).
6. **Ship.** `python3 tools/embed_net.py nets/net6.nnue`, bump the version, copy the sources
   into `Maharajah/Maharajah_ffi/src`, rebuild the site's wasm (`Maharajah_ffi/tools/build_wasm.sh`),
   and redo STRENGTH.md (`tools/nnue/ccrl_match.sh all`).

## Open / not done

- Engine 1.3.0 level 5 vs 1.2.0: SPRT stopped at 650 games, −14 Elo [−37, +8], inconclusive.
  Finish it on this machine before trusting 1.3.0's level 5:
  `python3 tools/match.py new/Maharajah old/Maharajah --tc 5+0.05 --arbiter TOOL --sprt=-10,0 --pairs 1200 --jobs 24`.
- Syzygy tablebases (3–5 pieces) for gen labels would be the next step up. They are not in the
  engine, and the app should not need them.
- Draw adjudication (±15 from ply 80 for 12 plies) is kept; it trusts the teacher in drawn
  endings, which 1.3.0 improves only for the cases in Endgame.h.
