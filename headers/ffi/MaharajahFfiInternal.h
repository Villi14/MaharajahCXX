#pragma once

#include "../Engine.h"
#include "../Search.h"

// C++-side access to the engine behind the mah_* functions, for native tools that
// drive the engine through the C interface but also need its internals
// (tools/maharajah_tool.cpp). Not part of the C interface.

namespace maharajah::ffi {

// The engine the mah_* functions operate on (created by mah_init() if needed).
Engine& live_engine();

// Nodes searched by the last mah_best_move_depth() / mah_best_move_time() call.
[[nodiscard]] u64 last_search_nodes();

// The whole result of that call: score (side to move, mate scores beyond
// +-Scores::mate_score) and depth of the last completed iteration.
[[nodiscard]] const SearchResult& last_search_result();

} // namespace maharajah::ffi
