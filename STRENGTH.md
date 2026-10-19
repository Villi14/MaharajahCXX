# How strong is Maharajah

We measured every difficulty level in matches against the chess engine Stash, whose versions
have official ratings on the **CCRL Blitz** list — a public rating list for chess programs.

| Level | Approximate rating |
|-------|--------------------|
| 1     | ~860               |
| 2     | ~1260              |
| 3     | ~1600              |
| 4     | ~2010              |
| 5     | **~2995**          |

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

Modern processors have several cores. With **Multi-threaded engine** turned on in the
settings, Maharajah thinks on several cores at once and plays stronger; the more cores, the
bigger the gain. On our test computer, level 5 on 4 cores scored 64 % against Stash 27.0
(where 1 core scored 40 %) — roughly **3150** on the same scale.

The cost is faster battery drain and a warmer phone during long games. If battery life matters
more, turn the switch off.

## How we measured

- Matches in cutechess-cli, time control 60 seconds + 0.6 seconds per move, 1 core, 64 MB hash.
- Computer: Intel Xeon E5-2680 v4 (2.4 GHz, turbo off).
- Openings from the 8moves_v3 book; every opening played with both colours.
- Level 5: 400 games each against Stash 25.0 (CCRL 2933) and Stash 27.0 (CCRL 3049).
  Levels 1–4: 100 games each against several Stash versions.
- Engine: commit cedff82 with the net5 network; anchor ratings from the CCRL Blitz list of
  2026-10-17.
- **[Download all games (PGN)](#)** <!-- link to the site's PGN archive -->

## Detailed results

Per match, from the side of Maharajah (wins – losses – draws). Rating = anchor rating + Elo
difference; the level's rating combines its anchors.

| Level | Skill | Anchor (CCRL) | Games | W – L – D | Score | Elo diff | Rating |
|-------|-------|---------------|-------|-----------|-------|----------|--------|
| 1 | 2  | Stash 9.0.1 (1271) | 100 | 7 – 90 – 3   | 8.5 %  | −413 ± 133 | ~860  |
| 2 | 4  | Stash 11.0.1 (1687) | 100 | 7 – 91 – 2  | 8.0 %  | −424 ± 142 | ~1260 |
| 3 | 6  | Stash 12.0 (1883)  | 100 | 9 – 89 – 2   | 10.0 % | −382 ± 124 | ~1500 |
| 3 | 6  | Stash 14.0 (2059)  | 100 | 9 – 88 – 3   | 10.5 % | −372 ± 118 | ~1690 |
| 4 | 8  | Stash 14.0 (2059)  | 100 | 40 – 52 – 8  | 44.0 % | −42 ± 67   | ~2017 |
| 4 | 8  | Stash 15.3 (2174)  | 100 | 24 – 73 – 3  | 25.5 % | −186 ± 79  | ~1988 |
| 4 | 8  | Stash 17.0 (2297)  | 100 | 14 – 80 – 6  | 17.0 % | −276 ± 90  | ~2021 |
| 5 | 10 | Stash 25.0 (2933)  | 400 | 207 – 120 – 73 | 60.9 % | +77 ± 31 | ~3010 |
| 5 | 10 | Stash 27.0 (3049)  | 400 | 121 – 201 – 78 | 40.0 % | −70 ± 31 | ~2979 |

Multi-threading check (not part of the 1-core list): level 5 with 4 threads vs Stash 27.0,
50 games, 2 at a time: 30 – 16 – 4, 64.0 %, about +100 ± 95 → ~3150, i.e. about +170 over
1 thread against the same anchor.

Matches where one side scored 0–1 % (level 1 vs Stash 11.0.1, level 2 vs Stash 12.0) were
played but give no usable estimate and are not counted. Error margins are 95 %.
