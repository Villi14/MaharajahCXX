# Engine strength for 1.0.1: plan (2026-10-08)

Status: **plan, not started.** Branch `engine-1.0.1`, here and in the monorepo
(`../Maharajah`, same plan in `docs/ideas/engine_1.0.1_plan_2026-10-08.md`). The engine work
happens in this repo (`TODO.md` is the measurement log) and is copied into the monorepo's
`Maharajah_ffi` for the release, as in monorepo 3df2f681.
Supersedes the "not scheduled" status of the monorepo's
`docs/ideas/nnue_architecture_balance_2026-10-04.md`; its analysis stands and is not repeated
here.

## Starting point (measured, `TODO.md`)

- net4_768: 1152 → 768 ×2 → SCReLU → 1, ~0.89 M parameters, ~68 M positions, 20 epochs,
  λ 0.75. No king buckets, no output buckets, no data augmentation.
- Each data round paid more than any search change: 3 → 10 M +168, net2 → net3 +157,
  net3 → net4 +102 (classic openings). Armies always gain less (+104 / +113 / +66).
- Round 4 (net4 as teacher) is generated in the background, 5 threads, ~0.36 M positions/h.
  At that rate 10 M ≈ 28 h of machine time, 30 M ≈ 3.5 days.
- Ladder (FSF-Elo, 520 ms/move): L1 1348, L2 1709, L3 1885, L4 2065, L5 2535. In the app L5
  thinks 5 s, i.e. stronger still.
- The app never calls `mah_set_threads` → always 1 thread. Lazy SMP 4 vs 1 = +70 [+15, +129].
- Search has: PVS, aspiration (gradual), null move (adaptive), LMR table, RFP, futility, LMP,
  IIR, check extension, killers, bounded history, TT move, SEE pruning in quiescence only.
  Search does NOT have: continuation / countermove history, an "improving" flag, SEE pruning
  in the main search, singular extensions, correction history.
- Rejected earlier, don't retry blindly: aged/bucketed TT, TT probe in quiescence (retry only
  after the TT keeps deeper entries), Texel tuning of the classic eval.

Every gain below is an **estimate to be measured**, not a result.

## Principle

The network is data-limited, data is slow to make, so first take everything that needs no new
data (search, trainer tricks on the existing 68 M), and let round 4 run meanwhile. One change =
one commit = one match, as before.

## Step 0 — make matches cheaper (prerequisite, ~1 day)

- **SPRT stop in `tools/match.py`** (open item in TODO). Bounds e.g. [0, 5] Elo for search
  changes, [0, 10] for networks. Most of the steps below are small gains; fixed 600–1200-game
  matches cost hours each and still end inside ±20.
- Freeze the baseline binary per step (already the rule: copy out of `build-release/`).

## Step 1 — search, no new data (each +5…+30 expected, ~1 day each incl. match)

In this order (cheapest and most reliable first):

1. **Continuation history** (1-ply and 2-ply: previous piece/to-square → this move), into move
   ordering and LMR. Usually the biggest single search gain once history exists.
2. **"Improving" flag** (static eval vs 2 plies ago) in RFP margin, LMP count and LMR. Cheap
   now that NNUE eval is trustworthy.
3. **SEE pruning in the main search**: skip quiets with SEE < −k·depth² and captures with
   SEE < −k·depth at shallow depth. Compound pieces make SEE swings bigger — check
   `See.cpp` values for A/C/M before tuning k.
4. **Singular extensions** (TT move extended when all others fail low at reduced depth).
   Bigger gain, more risk; only after 1–3.
5. **Correction history** (static eval corrected by pawn-structure key) — last, optional.

Measure both opening sets (`--custom-share 0` and `1`) — armies are the product.
Expected total of step 1: +40…+100 in self-play, which shows up much smaller against FSF.

## Step 2 — the trainer, on the existing 68 M (no new data)

1. **Horizontal mirror augmentation** (a↔h). Doubles the data for free in every position
   where no castling right is left (castling is the only file-asymmetric rule — verify against
   `../Maharajah/docs/MAHARAJAH_RULES.md` before coding; field 8 pawn squares mirror too). Since the net is
   data-limited this is the most promising cheap item. Done in `make_batch`, no format change.
2. **λ sweep** 0.5 / 0.75 / 1.0 and epochs 20 → 30 at 768 hidden. Pure retraining (~8 h each).
3. **Output buckets by material** (8 buckets by piece count or army points), file format v2,
   `embed_net.py`, trainer output layer. No speed cost. Targets the weak point: custom armies
   have far more varied material than orthodox chess.
4. **King safety: king buckets with king mirroring** (owner, 2026-10-08: in 1.0.1 as an
   experiment, not postponed). Today the net sees "piece X on square Y" but not where its own
   king stands, so a queen or an amazon next to the king weighs the same as far from it — and
   compound pieces make king attacks far more dangerous than in orthodox chess (assumption, to
   be measured). Kept small for 68 M positions:
   - **2 zones** of the own king's square (e.g. ranks 1–2 vs further up, per perspective), not 4:
     input weights ×2 (~1.8 M parameters, ~38 positions per parameter), not ×4 (~19).
   - **King mirroring** (Stockfish's "hm"): king on files e–h ⇒ the perspective's features are
     mirrored so its king is always on a–d. Not while that side still has a castling right
     (castling is file-asymmetric). Same mirror code as item 1.
   - Accumulator width unchanged; a king move across a zone or mirror boundary forces a full
     refresh for that perspective (rare). Variant: kings can start off e1/e8 — zones by the
     king's actual square, never by the orthodox one.
   - File format v2 (together with item 3): zone count and mirroring flag in the header.
   - If net5c does not beat net5b on both opening sets, postpone to 1.0.2 with more data.

Networks, each matched against the previous one (300 pairs × 2, 50 ms/move, both opening sets):
`net5a` = net4 data + mirror + best λ; `net5b` = + output buckets; `net5c` = + king buckets.

## Step 3 — round 4 data + next network

- When round 4 reaches ≥ 10 M: `net5` on all data (~78 M+, ×2 with mirroring).
- **Raise the army share of new data** (`custom_share` 0.5 → ~0.65): the army gain lags every
  round, and armies are what makes the game ours.
- **Packed binary data format** (~40 bytes/position, streamed) before going past ~150 M —
  already listed in TODO; with mirroring the text reader becomes the limit sooner.
- `net5` = the best of net5a/b/c's architecture on all data. More king zones (4) only once the
  total is ≥ ~150 M effective positions — 1.0.2.
- Online games are **not** training data: prod has 1 real game. Keep them for checking the
  daily analysis, not for training.

## Step 4 — product side (what the player actually feels)

1. **Threads at L5 only** in the app: `mah_set_threads(2)` (4 on desktop). L1–L4 are depth-
   capped, threads give them nothing. Measure on the Nokia 6.1: time per move, heat, battery
   over 10 games. Desktop gain ~+70; phones less.
2. **Retune the ladder** after the new net (it moves every level): the gaps are L1→L2 +361 and
   L4→L5 +470; L1 at ~1350 is likely too strong for beginners. Rerun
   `~/nnue-data/ladder/ladder.sh`, then adjust `skill_profiles` / `ui_to_skill_level` and the
   think times in the app's `Maharajah_ui/lib/logic/engine_policy.dart`. Target roughly even steps (e.g. 1100 / 1450 / 1750 /
   2050 / 2400+).
3. **Weak devices**: on the Nokia NNUE is already slower than classic per search. A bigger net
   is ruled out; if output buckets or king buckets cost speed there, keep the 768 width.

## Release gate for 1.0.1 (engine part)

- ctest green here (incl. ASan/UBSan), `compare_perft.py` 0 mismatches.
- New net vs net4_768: positive on both opening sets with the interval above 0.
- FSF anchor rerun at L5 (520 ms, 2500/2700 anchors) and the full ladder.
- Bench timings: x86, WASM SIMD, Nokia 6.1 — no level slower than in 1.0.0 by more than ~10 %.
- Engine copied into `Maharajah_ffi`, app + site WASM + server `.so` + `maharajah_tool`
  rebuilt; bump `EngineVersion` so `/Admin/Games` re-analyses with the new engine.
- Version 1.0.1 for app/engine/server as in monorepo e7087330 (and here as in 71a2100).

## Suggested order and rough calendar

| # | item | needs | est. |
|---|---|---|---|
| 0 | SPRT in match.py | — | 1 d |
| 1 | continuation history, improving, SEE pruning | 0 | 3 d |
| 2 | mirror augmentation + λ sweep → net5a | — (runs parallel to 1) | 2–3 d machine |
| 3 | output buckets (format v2) → net5b | 2 | 2 d + training |
| 4 | king buckets (2 zones + king mirroring) → net5c | 3 | 2 d + training |
| 5 | singular extensions | 1 | 1–2 d |
| 6 | round 4 ≥ 10 M → net5 | background | when ready |
| 7 | L5 threads + ladder retune + device bench | 6 | 2 d |
| 8 | sync to Maharajah_ffi, rebuild all, release | 7 | 1 d |

Items 1 and 2 don't touch each other (search vs trainer), so they run side by side: the CPU
trains while matches run on the other machine.

## Open questions for the owner

- Is the goal a stronger L5, or a better-spaced ladder for ordinary players? (Step 4.2 matters
  more for players than +50 at the top.)
- Threads on phones: acceptable battery cost at L5?
- ~~King buckets in 1.0.1?~~ Decided 2026-10-08: yes, as experiment step 2.4 (2 zones + king
  mirroring).
