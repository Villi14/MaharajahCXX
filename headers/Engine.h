#pragma once

#include "Board.h"
#include "EngineConfig.h"
#include "Nnue.h"
#include "Transposition.h"
#include "Zobrist.h"

#include <atomic>
#include <functional>

namespace maharajah {

struct TimeControl {
  bool timeset{ };
  int starttime{ };
  // hard limit: the search stops even inside an iteration
  int stoptime{ };
  // soft limit (clock mode, 0 = none): no new iteration starts after it
  int soft_stoptime{ };
  // set by the main search thread (time up, "stop") and read by every search thread
  std::atomic<bool> stopped{ };
  // nodes searched so far by helper threads, for the main thread's "info" lines
  std::atomic<u64> helper_nodes{ };
  bool quit{ };
  // check for "stop"/"quit" input while searching (UCI searches without a fixed depth)
  bool poll_input{ };
  std::function<void(TimeControl&)> poll{ };

  // clears the limits and flags; keeps the poll callback
  void reset() {
    timeset = false;
    starttime = 0;
    stoptime = 0;
    soft_stoptime = 0;
    stopped = false;
    quit = false;
    poll_input = false;
  }

  // limits the search to `movetime_ms` from now (no limit when <= 0)
  void set_movetime(int movetime_ms);
};

// Everything that persists between searches: the position, the hash table,
// strength settings and the random stream used by weaker skill levels.
struct Engine {
  static constexpr int min_threads{ 1 };
  static constexpr int max_threads{ 64 };

  Engine();

  Board board{ };
  TranspositionTable transposition_table{ };
  SearchConfig search_config{ SearchConfig::for_difficulty(SearchConfig::max_difficulty) };
  EvalConfig eval_config{ };
  Random random{ };
  TimeControl time_control{ };
  Nnue nnue{ };
  // search threads; more than one is nondeterministic (see run_search)
  int threads{ min_threads };

  // Loads a FEN; a position without exactly one king per side is rejected
  // (returns false) and the start position is loaded instead. Throws on a malformed FEN.
  bool set_position(std::string_view fen);
  // Plays a move given in coordinate notation, remembering the previous position for
  // repetition detection. Returns false (and changes nothing) if it is not legal.
  bool apply_move(std::string_view move);
};

} // namespace maharajah
