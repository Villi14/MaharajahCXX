# How strong is Maharajah

We measured every difficulty level in matches against the chess engine Stash, whose versions
have official ratings on the **CCRL Blitz** list — a public rating list for chess programs.

| Level | Approximate rating |
|-------|--------------------|
| 1     | ~910               |
| 2     | ~1285              |
| 3     | ~1630              |
| 4     | ~2035              |
| 5     | **~2975**          |

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
bigger the gain. On our test computer, level 5 on 4 cores scored 64 % against Stash 27.0
(where 1 core scored 40 %) — roughly **3150** on the same scale (measured on version 1.1.0).

The cost is faster battery drain and a warmer phone during long games. If battery life matters
more, turn the switch off.

## How we measured

- Matches in cutechess-cli 1.4.0, time control 60 seconds + 0.6 seconds per move, 1 core, 64 MB hash.
- Computer: Intel Xeon E5-2680 v4 (2.4 GHz, turbo off).
- Openings from the 8moves_v3 book; every opening played with both colours.
- Level 5: 400 games each against Stash 25.0 (CCRL 2933) and Stash 27.0 (CCRL 3049).
  Levels 1–4: 100 games each against several Stash versions.
- Engine: Maharajah 1.3.1 (commit ec4659a) with the net5 network, played on 2026-11-10;
  anchor ratings from the CCRL Blitz list of 2026-10-17.
- The matches were played with [tools/nnue/ccrl_match.sh](tools/nnue/ccrl_match.sh), which
  lists the full cutechess-cli command (`ccrl_match.sh all`).
- **[Download all games (PGN)](#)** <!-- link to the site's PGN archive -->

## Detailed results

Per match, from the side of Maharajah (wins – losses – draws). Rating = anchor rating + Elo
difference; the level's rating combines its anchors.

| Level | Skill | Anchor (CCRL) | Games | W – L – D | Score | Elo diff | Rating |
|-------|-------|---------------|-------|-----------|-------|----------|--------|
| 1 | 2  | Stash 9.0.1 (1271) | 100 | 11 – 89 – 0  | 11.0 % | −363 ± 121 | ~908  |
| 2 | 4  | Stash 11.0.1 (1687) | 100 | 8 – 90 – 2  | 9.0 %  | −402 ± 132 | ~1285 |
| 3 | 6  | Stash 12.0 (1883)  | 100 | 11 – 87 – 2  | 12.0 % | −346 ± 112 | ~1537 |
| 3 | 6  | Stash 14.0 (2059)  | 100 | 11 – 86 – 3  | 12.5 % | −338 ± 108 | ~1721 |
| 4 | 8  | Stash 14.0 (2059)  | 100 | 39 – 52 – 9  | 43.5 % | −45 ± 66   | ~2014 |
| 4 | 8  | Stash 15.3 (2174)  | 100 | 26 – 67 – 7  | 29.5 % | −151 ± 73  | ~2023 |
| 4 | 8  | Stash 17.0 (2297)  | 100 | 18 – 75 – 7  | 21.5 % | −225 ± 81  | ~2072 |
| 5 | 10 | Stash 25.0 (2933)  | 400 | 185 – 138 – 77 | 55.9 % | +41 ± 31 | ~2974 |
| 5 | 10 | Stash 27.0 (3049)  | 400 | 122 – 206 – 72 | 39.5 % | −74 ± 31 | ~2975 |

Multi-threading check (not part of the 1-core list, measured on version 1.1.0): level 5 with 4 threads vs Stash 27.0,
50 games, 2 at a time: 30 – 16 – 4, 64.0 %, about +100 ± 95 → ~3150, i.e. about +170 over
1 thread against the same anchor.

Matches where one side scored 0–1 % (level 1 vs Stash 11.0.1, level 2 vs Stash 12.0) were
played but give no usable estimate and are not counted. Error margins are 95 %.

Compared with version 1.1.0 (same protocol, 2026-10-19), levels 1–4 gained about 20–50 points
(they now play small won endgames at full strength); level 5 is unchanged within the error
(2995 ± 22 then, 2975 ± 22 now).
