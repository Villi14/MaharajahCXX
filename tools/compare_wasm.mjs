// Loads MaharajahC's and this engine's browser modules (maharajah_engine.js from
// Maharajah_ffi/tool/build_wasm.sh and tools/build_wasm.sh) in Node and drives both
// through the same mah_wasm_* calls: best moves at fixed depths, self-play games,
// FEN round trips, game status and rejected input. Exit code 1 on a mismatch.
//
// Usage: node tools/compare_wasm.mjs <C module .js> <C++ module .js> [--quick]

import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const args = process.argv.slice(2);
const quick = args.includes('--quick');
const [cPath, cxxPath] = args.filter((arg) => !arg.startsWith('--'));
if (!cPath || !cxxPath) {
  console.error('usage: node tools/compare_wasm.mjs <C module .js> <C++ module .js> [--quick]');
  process.exit(2);
}

async function load(path) {
  const factory = (await import(pathToFileURL(resolve(path)).href)).default;
  const module = await factory();
  return {
    init: module.cwrap('mah_wasm_init', 'number', []),
    setPositionFen: module.cwrap('mah_wasm_set_position_fen', 'number', ['string']),
    applyMove: module.cwrap('mah_wasm_apply_move', 'number', ['string']),
    gameStatus: module.cwrap('mah_wasm_game_status', 'number', []),
    getFen: module.cwrap('mah_wasm_get_fen', 'string', ['number']),
    bestMoveDepth: module.cwrap('mah_wasm_best_move_depth', 'string', ['number']),
    bestMoveTime: module.cwrap('mah_wasm_best_move_time', 'string', ['number']),
  };
}

// the position lists of compare_engines.py, so both comparisons cover the same FENs
function positions(name) {
  const source = readFileSync(join(dirname(fileURLToPath(import.meta.url)), 'compare_engines.py'), 'utf8');
  const list = source.match(new RegExp(`^${name} = \\[([\\s\\S]*?)^\\]`, 'm'))[1];
  return [...list.matchAll(/"([^"]+)"/g)].map((match) => match[1]);
}

const engines = [await load(cPath), await load(cxxPath)];
let checks = 0;
const mismatches = [];

// calls `fn` on both engines and records a mismatch if the results differ
function both(label, fn) {
  const [c, cxx] = engines.map(fn);
  checks++;
  if (c !== cxx) mismatches.push({ label, c, cxx });
  return cxx;
}

for (const engine of engines) engine.init();

const fens = [...positions('STANDARD_FENS'), ...positions('VARIANT_FENS')];
const depths = quick ? [1, 3, 5] : [1, 2, 3, 4, 5, 6, 7];

for (const fen of fens) {
  both(`set ${fen}`, (e) => e.setPositionFen(fen));
  both(`get_fen ${fen}`, (e) => e.getFen(1));
  both(`status ${fen}`, (e) => e.gameStatus());
  for (const depth of depths) {
    both(`depth ${depth}: ${fen}`, (e) => {
      e.setPositionFen(fen);
      return e.bestMoveDepth(depth);
    });
  }
}

// self-play: both engines play the move the C++ engine chose, so a single
// mismatch does not cascade into every later ply
const plies = quick ? 20 : 80;
for (const fen of fens) {
  for (const engine of engines) engine.setPositionFen(fen);
  for (let ply = 0; ply < plies; ply++) {
    const move = both(`selfplay ply ${ply} move: ${fen}`, (e) => e.bestMoveDepth(4));
    if (!move) break;
    both(`selfplay ply ${ply} apply ${move}: ${fen}`, (e) => e.applyMove(move));
    both(`selfplay ply ${ply} fen: ${fen}`, (e) => e.getFen(ply + 1));
    if (both(`selfplay ply ${ply} status: ${fen}`, (e) => e.gameStatus()) !== 0) break;
  }
}

const rejected = ['', 'not a fen', '8/8/8/8/8/8/8/8 w - - 0 1', 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'];
for (const fen of rejected) {
  both(`set rejected ${JSON.stringify(fen)}`, (e) => e.setPositionFen(fen));
  both(`get_fen after ${JSON.stringify(fen)}`, (e) => e.getFen(1));
  for (const move of ['', 'e2e5', 'e7e5', 'zz', 'e2e4']) both(`apply ${move} after ${JSON.stringify(fen)}`, (e) => e.applyMove(move));
}

// timed searches differ by the clock, so only check that each returns a legal move
for (const [index, engine] of engines.entries()) {
  engine.setPositionFen(fens[0]);
  const move = engine.bestMoveTime(100);
  checks++;
  if (!move || !engine.applyMove(move)) mismatches.push({ label: `best_move_time (${index ? 'C++' : 'C'})`, c: move, cxx: move });
}

for (const { label, c, cxx } of mismatches.slice(0, 20)) console.log(`MISMATCH ${label}\n  C:   ${c}\n  C++: ${cxx}`);
console.log(`${checks} checks over ${fens.length} positions, ${mismatches.length} mismatches`);
process.exit(mismatches.length ? 1 : 0);
