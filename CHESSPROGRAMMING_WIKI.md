# Maharajah on the Chess Programming Wiki

A draft page for [chessprogramming.org](https://www.chessprogramming.org). Written on
2026-11-11. On that date the wiki ran as a read-only recovery copy, so the page cannot be
created yet.

## How to publish it once editing is open again

1. Create an account on the wiki (editing needs a login; new accounts may wait for approval).
2. Open `https://www.chessprogramming.org/index.php?title=Maharajah&action=edit`, paste the
   text below and preview it.
   - Before saving, check that the page name `Maharajah` is still free and that every red link in
     the preview is worth keeping. Red links on CPW are normal; `[[Oleksandr Velykotskyi]]`
     stays red until an author page exists.
   - Categories that do not exist yet show in red too. Remove those or change them to the
     names the wiki uses.
3. Add the engine to the manually kept alphabetical list on
   [Engines](https://www.chessprogramming.org/Engines), under **M**:
   `* [[Maharajah]] by [[Oleksandr Velykotskyi]]`
4. Optional: add `* [[Maharajah]]` under "See also" on the [[BBC]] page, as is done for
   other engines derived from it.
5. When CCRL or CEGT publish a rating, add it under "External Links → Rating Lists".

## Wiki text

```
'''[[Main Page|Home]] * [[Engines]] * Maharajah'''

'''Maharajah''',<br/>
an [[Open Source Engines|open source chess engine]] by [[Oleksandr Velykotskyi]], written in [[Cpp|C++]] (C++23) and licensed under the [[Massachusetts Institute of Technology#License|MIT License]].
Maharajah is [[UCI]] compatible and also exports a C interface used by the author's chess apps, site and server.
It derives from [[BBC]] 1.2 by [[Maksim Korzh]] <ref>[https://github.com/maksimKorzh/bbc/blob/master/src/old_versions/bbc_1.2.c bbc/bbc_1.2.c at master · maksimKorzh/bbc · GitHub]</ref>, via the author's C engine MaharajahC <ref>[https://github.com/Villi14/MaharajahC GitHub - Villi14/MaharajahC]</ref>, and was ported to C++ in 2026.
The engine also plays [[Chess Variants|variants]] with the compound pieces Archbishop (bishop + knight), Chancellor (rook + knight) and Amazon or "Maharajah" (queen + knight), which are handled throughout move generation, [[Static Exchange Evaluation|SEE]], evaluation and draw rules.
Version 1.0.0 was the first release on GitHub, in October 2026 <ref>[https://github.com/Villi14/MaharajahCXX/releases Releases · Villi14/MaharajahCXX · GitHub]</ref>.

=Features=
==[[Board Representation]]==
* [[Bitboards]]
* [[Magic Bitboards]]
* [[Encoding Moves|Bit-packed move encoding]]

==[[Search]]==
* [[Iterative Deepening]]
* [[Principal Variation Search]]
* [[Aspiration Windows]]
* [[Transposition Table]], lockless, shared by threads
* [[Lazy SMP]]
* [[Null Move Pruning]], only with at least two pieces besides king and pawns
* [[Reverse Futility Pruning]]
* [[Internal Iterative Reductions]]
* [[Check Extensions]]
* [[Late Move Reductions]]
* [[Futility Pruning]]
* [[Futility Pruning#MoveCountBasedPruning|Late Move Pruning]]
* [[Move Ordering]]
** [[MVV-LVA]]
** [[Killer Heuristic]]
** [[History Heuristic]]
* [[Quiescence Search]] with [[Static Exchange Evaluation|SEE]] pruning

==[[Evaluation]]==
* [[NNUE]], embedded in the executable
** 18 piece kinds × 64 squares → 768 × 2 perspectives → SCReLU → 1; the compound pieces have inputs of their own
** trained only on the engine's own self-play games
** [[SIMD and SWAR Techniques|SIMD]] accumulator updates (SSE2, AVX2, NEON), adapted from the nnue library by [[Jon Dart]]
* the hand-written evaluation inherited from BBC remains as a fallback (UCI option ''UseNNUE'')

=See also=
* [[BBC]]

=External Links=
==Chess Engine==
* [https://github.com/Villi14/MaharajahCXX GitHub - Villi14/MaharajahCXX]
* [https://github.com/Villi14/MaharajahCXX/releases Releases · Villi14/MaharajahCXX · GitHub]

=References=
<references />

'''[[Engines|Up one level]]'''
[[Category:UCI]]
[[Category:Open Source]]
[[Category:MIT]]
[[Category:Cpp]]
[[Category:NNUE]]
[[Category:Windows]]
[[Category:Linux]]
[[Category:Mac]]
[[Category:X64]]
[[Category:ARM]]
```
