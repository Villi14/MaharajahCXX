# How strong is Maharajah

We measured every difficulty level in matches against the chess engine Stash, whose versions
have official ratings on the **CCRL Blitz** list — a public rating list for chess programs.

| Level | Approximate rating |
|-------|--------------------|
| 1     | ~985               |
| 2     | ~1325              |
| 3     | ~1725              |
| 4     | ~2100              |
| 5     | **~3075**          |

Levels 1–4 play with deliberate limits and suit players from beginners to strong club players.
Level 5 is the engine at full strength, with no handicaps.

> The CCRL rating is a scale for chess programs, not for people, so it cannot be compared
> directly with a FIDE rating.

## Why level 5 is weaker on a phone

Levels 1–4 play the same on any device.

At level 5 the engine thinks for the same time on every move, so **the faster the processor,
the deeper it calculates and the stronger it plays**. The rating in the table was measured on
a computer. On a phone level 5 is usually weaker — noticeably on older and budget models,
hardly at all on new flagships.

Even on the slowest phone, level 5 is far stronger than any human player.

## Multi-threading

Modern processors have several cores. With **Multi-threaded Engine** turned on in the
settings, Maharajah thinks on several cores at once and plays stronger; the more cores, the
bigger the gain. On our test computer, level 5 on 4 cores scored 49 % against Stash 31.0
(CCRL 3207) — roughly **3200** on the same scale, about 125 points above 1 core.

The cost is faster battery drain and a warmer phone during long games. If battery life matters
more, turn the switch off.

## How we measured

- Matches in cutechess-cli 1.5.1, time control 60 seconds + 0.6 seconds per move, 1 core, 64 MB hash.
- Computer: Intel Xeon E5-2680 v4 (2.4 GHz, turbo off).
- Openings from the 8moves_v3 book; every opening played with both colours.
- Level 5: 400 games each against Stash 25.0 (CCRL 2933) and Stash 27.0 (CCRL 3049).
  Levels 1–4: 100 games each against several Stash versions.
- Engine: Maharajah 1.4.0 (commit 3ab5b58) with the net5 network, played on 2026-11-15;
  anchor ratings from the CCRL Blitz list of 2026-10-17.
- The matches were played with [tools/nnue/ccrl_match.sh](tools/nnue/ccrl_match.sh), which
  lists the full cutechess-cli command (`ccrl_match.sh all`).
- **[Download all games (PGN)](#)** <!-- link to the site's PGN archive -->

## Detailed results

Per match, from the side of Maharajah (wins – losses – draws). Rating = anchor rating + Elo
difference; the level's rating combines its anchors.

| Level | Skill | Anchor (CCRL) | Games | W – L – D | Score | Elo diff | Rating |
|-------|-------|---------------|-------|-----------|-------|----------|--------|
| 1 | 2  | Stash 9.0.1 (1271) | 100 | 15 – 83 – 2  | 16.0 % | −288 ± 97  | ~983  |
| 2 | 4  | Stash 11.0.1 (1687) | 100 | 10 – 88 – 2 | 11.0 % | −363 ± 117 | ~1324 |
| 3 | 6  | Stash 12.0 (1883)  | 100 | 28 – 68 – 4  | 30.0 % | −147 ± 74  | ~1736 |
| 3 | 6  | Stash 14.0 (2059)  | 100 | 10 – 86 – 4  | 12.0 % | −346 ± 108 | ~1713 |
| 4 | 8  | Stash 14.0 (2059)  | 100 | 46 – 48 – 6  | 49.0 % | −7 ± 67    | ~2052 |
| 4 | 8  | Stash 15.3 (2174)  | 100 | 36 – 56 – 8  | 40.0 % | −70 ± 67   | ~2104 |
| 4 | 8  | Stash 17.0 (2297)  | 100 | 25 – 68 – 7  | 28.5 % | −160 ± 74  | ~2137 |
| 5 | 10 | Stash 25.0 (2933)  | 400 | 228 – 85 – 87  | 67.9 % | +130 ± 32 | ~3063 |
| 5 | 10 | Stash 27.0 (3049)  | 400 | 173 – 126 – 101 | 55.9 % | +41 ± 30 | ~3090 |

Multi-threading check (not part of the 1-core list): level 5 with 4 threads vs Stash 31.0
(CCRL 3207 on the CCRL Blitz list of 2026-11-14), 100 games, 3 at a time: 31 – 33 – 36, 49.0 %,
−7 ± 55 → ~3200, i.e. about +125 over 1 thread (3075). Stash 31.0 was built from its source
at the v31.0 release (bench 13 162 331). Version 1.1.0 measured ~3150 on 4 threads against
Stash 27.0 (50 games, ± 95).

Matches where one side scored 5 % or less (level 1 vs Stash 11.0.1: 1 – 99 – 0; level 2 vs
Stash 12.0: 4 – 94 – 2) were played but give no usable estimate and are not counted. Error
margins are 95 %.

Compared with version 1.3.1 (same protocol and anchors, 2026-11-10: ~910 / ~1285 / ~1630 /
~2035 / 2975 ± 22), version 1.4.0 is about **+100** at level 5 (3075 ± 22) and about +40 to
+95 at levels 1–4. The gain comes from the search: the check extension, the move ordering
and the repetition rule apply at every level, the deeper pruning at level 5 only. Each level 1–4 figure alone is
within its error (100 games a pair), so read the ladder as a rough guide.
