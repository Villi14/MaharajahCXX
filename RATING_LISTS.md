# Getting Maharajah onto public rating lists

Notes of 2026-11-11, written after release 1.3.1 and updated for 1.3.2 and 1.4.0. Our own estimate of the engine's strength is
in [STRENGTH.md](STRENGTH.md). This file is about getting it tested by independent rating
lists such as CCRL and CEGT.

## How an engine gets tested

There is no registration form. The testers choose which engines to run. An author announces
a release on a forum and asks for testing:

- The usual place is the "New engine releases & news" thread on
  [TalkChess](https://talkchess.com). CCRL, CEGT and other testers read it. The public CCRL
  forum works too.
- Someone from the CCRL team (for example Gabor Szots) answers and puts the engine in the
  queue. The queue is long, and not every version gets tested.
- Only standard chess is tested. The compound pieces play no part in these lists.

## What to put in the announcement

- Name, author (`id author Villi`), country, protocol (UCI).
- A link to the GitHub release with the Windows builds (`windows-x64`, `windows-x64-avx2`).
- The lists we ask for: CCRL Blitz (2' + 1"), CCRL 40/15, and maybe 4 CPU.
- Our estimate as a guide: about 3075 on the CCRL Blitz scale at full strength
  (STRENGTH.md, measured on 1.4.0 with the SSE2 `x64` build; 1.3.1 was about 2975). The
  `x64-avx2` build is about 18 % faster: say so, as their figure for it may come out higher.
- The origin, stated openly: the engine derives from BBC 1.2 by Maksim Korzh (through
  MaharajahC and the C++ port). The NNUE network is our own, trained on our own data.
  Forums are sensitive about clones, and an open origin (as in the README Credits) avoids
  questions.

## Checks before announcing

| Item | Status |
|---|---|
| Windows builds on GitHub | Done (release 1.4.0, built by .github/workflows/release.yml) |
| UCI options `Hash`, `Threads`, `Skill Level` (default 10, full strength) | Done |
| `Hash` maximum | 1024 MB since 1.3.2 (was 128, inherited from BBC; testers' larger values were cut to 128) |
| One UCI block, on `uci` only | Since 1.3.2; before, the engine also printed its id, options and `uciok` unasked at start |
| Time management at 40/15 (repeating control, `movestogo`) | Fixed in 1.3.2. 1.3.1 against Stash 25.0, 112 games each, cutechess `timemargin=0`: 22 losses on time at 40/20 and 29 at 40/5, every one on move 40, 80 or 120 (`movestogo 1`, the whole clock planned). 1.3.2 keeps 50 ms + 5 % of the clock: 0 losses on time at both, +53 ± 59 and −9 ± 59 Elo |
| Time management in 1.4.0 (new UCI handling during a search) | Checked 2026-11-15, same test: 0 losses on time at 40/20 and 40/5, +112 ± 63 and +130 ± 65 Elo against Stash 25.0 |

Submit 1.4.0, never 1.3.1: at 40/15 1.3.1 would lose about one game in five on time.

## Lists that already test Maharajah

### Computer Chess Index (CCI)

[CCI](https://github.com/computer-chess-index/cci) is a rating list run by one person, who
follows new releases on GitHub (they starred the repository) and tests them without being
asked. Maharajah's page: [engines/Maharajah.md](https://github.com/computer-chess-index/cci/blob/main/engines/Maharajah.md).

Ratings as of 2026-11-21, one thread, Intel Core Ultra 7 265T (1.50 GHz), E-core:

| Version | STC 8+0.08s | LTC 60+0.6s | VLTC 2'24"+1.12s |
|---|---|---|---|
| 1.3.2 | 2394 | 2726 | 2898 |
| 1.4.0 | 2553 | 2866 | 2948 |
| 1.4.1 | 2595 | 2878 | 3029 |

On the P-core of the same CPU, 1.4.1 has 2614 / 2882 / 2928.

- The scale is CCI's own. BayesElo is anchored to a random mover at 0, and the result is
  multiplied by 1.35. The author writes that the scale is compressed and not yet calibrated
  against CCRL, so these figures are not comparable with our CCRL estimate in STRENGTH.md.
- All three time controls play in one tournament, so the columns are comparable with each
  other: 1.4.1 gains about 430 from STC to VLTC.
- Conditions: cutechess-cli, book `8moves_v3`, no pondering, one thread.
- New versions can also be submitted through the "Submit new version" link on the engine page.
- CCI accepts a voluntary declaration of AI use in an engine's development (an issue
  template, section 5 of its [METHODOLOGY.md](https://github.com/computer-chess-index/cci/blob/main/METHODOLOGY.md)).
  None has been submitted for Maharajah.

## A faster public rating

The engine can run as a Lichess bot through [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot).
A rating appears within days, but it is not comparable with CCRL.

## References

- [TalkChess: New engine releases & news 2021](https://talkchess.com/forum3/viewtopic.php?p=901458)
- [TalkChess: New engine releases & news H1 2022](https://talkchess.com/forum3/viewtopic.php?p=918934)
- [TalkChess: a post copied from the CCRL forum](https://talkchess.com/forum3/viewtopic.php?p=753042)
