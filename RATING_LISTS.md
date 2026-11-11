# Getting Maharajah onto public rating lists

Notes of 2026-11-11, written after release 1.3.1. Our own estimate of the engine's strength is
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
- Our estimate as a guide: about 2975 on the CCRL Blitz scale at full strength
  (STRENGTH.md, measured on 1.3.1).
- The origin, stated openly: the engine derives from BBC 1.2 by Maksim Korzh (through
  MaharajahC and the C++ port). The NNUE network is our own, trained on our own data.
  Forums are sensitive about clones, and an open origin (as in the README Credits) avoids
  questions.

## Checks before announcing

| Item | Status |
|---|---|
| Windows builds on GitHub | Done (release 1.3.1) |
| UCI options `Hash`, `Threads`, `Skill Level` (default 10, full strength) | Done |
| `Hash` above the maximum | Checked on 1.3.1: `setoption name Hash value 256` does not crash; the engine plays with 128 MB |
| Raise the `Hash` maximum (128 MB now, in `sources/Game.cpp`) to at least 1024 MB | Open, for the next version (e.g. 1.3.2); testers set their standard hash, which may be larger |
| Time management at 40/15 (repeating control, `movestogo`) | Open: play a few dozen cutechess games at that control and check for time losses. `tools/nnue/ccrl_match.sh` has only been run at 60 + 0.6 |

Release 1.3.1 can be submitted as is. The two open items are improvements, not bugs, and do
not need a release of their own.

## A faster public rating

The engine can run as a Lichess bot through [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot).
A rating appears within days, but it is not comparable with CCRL.

## References

- [TalkChess: New engine releases & news 2021](https://talkchess.com/forum3/viewtopic.php?p=901458)
- [TalkChess: New engine releases & news H1 2022](https://talkchess.com/forum3/viewtopic.php?p=918934)
- [TalkChess: a post copied from the CCRL forum](https://talkchess.com/forum3/viewtopic.php?p=753042)
