#include "../../headers/CustomSetup.h"
#include "../../headers/Engine.h"
#include "../../headers/Search.h"
#include "../../headers/ffi/MaharajahFfiInternal.h"
#include "../../headers/ffi/maharajah_ffi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

using namespace maharajah;
using maharajah::ffi::live_engine;

namespace {

constexpr int ffi_min_hash_mb{ 4 };
constexpr int ffi_max_hash_mb{ 1024 };

std::unique_ptr<Engine> engine;
u64 last_nodes{ };

int copy_out_string(const std::string& value, char* out, const int out_len) {
  if(out == nullptr || out_len <= 0 || static_cast<std::size_t>(out_len) < value.size() + 1)
    return 0;

  std::memcpy(out, value.c_str(), value.size() + 1);
  return 1;
}

int copy_out_move(const int move, char* out_move, const int out_len) {
  if(move == 0)
    return 0;
  return copy_out_string(Board::move_to_string(move), out_move, out_len);
}

bool nnue_requested(const Engine& live) {
  return live.eval_config.eval_mode == EvalMode::nnue;
}

// the search evaluates with the network when NNUE is requested and weights are loaded
bool nnue_active(const Engine& live) {
  return nnue_requested(live) && live.nnue.has_loaded_weights();
}

const char* eval_status_string(const Engine& live) {
  // NNUE requested without weights falls back to the classic evaluation
  return nnue_active(live) ? "nnue_ready" : "classic_ready";
}

// the app owns stdin, so FFI searches never poll it
int search_best_move(Engine& live, const int depth) {
  live.time_control.poll_input = false;
  const SearchResult result = run_search(live, depth);
  last_nodes = result.nodes;
  return result.best_move;
}

// auto keeps what the FEN says (including per-side variant rights); the explicit
// profiles pin both sides regardless of the material on the board
void apply_rules_profile(Engine& live, const int rules_profile) {
  BoardState& state = live.board.state;

  if(rules_profile == mah_rules_variant) {
    state.standard_rules = false;
    state.castle = 0;
    state.side_variant = { true, true };
    state.infer_pawn_state();
    state.hash_key = generate_hash_key(state);
    return;
  }

  if(rules_profile == mah_rules_standard && !state.has_compound_pieces()) {
    state.standard_rules = true;
    state.side_variant = { false, false };
    state.hash_key = generate_hash_key(state);
  }
}

} // namespace

Engine& maharajah::ffi::live_engine() {
  if(!engine)
    mah_init();
  return *engine;
}

u64 maharajah::ffi::last_search_nodes() {
  return last_nodes;
}

FFI_PLUGIN_EXPORT int mah_init(void) {
  if(!engine)
    engine = std::make_unique<Engine>();

  engine->search_config = SearchConfig::for_difficulty(SearchConfig::max_difficulty);
  engine->eval_config.eval_mode = EvalMode::nnue;
  if(!engine->nnue.has_loaded_weights())
    engine->nnue.load_default_weights();
  engine->board.parse_fen(Fen::start_position);
  engine->transposition_table.clear();
  return 1;
}

FFI_PLUGIN_EXPORT int mah_set_position_fen(const char* fen) {
  return mah_set_position_fen_with_rules(fen, mah_rules_auto);
}

FFI_PLUGIN_EXPORT int mah_set_position_fen_with_rules(const char* fen, const int rules_profile) {
  if(fen == nullptr || *fen == '\0')
    return 0;

  Engine& live = live_engine();
  bool valid;
  try {
    valid = live.set_position(fen);
  } catch(const std::runtime_error&) {
    live.board.parse_fen(Fen::start_position);
    valid = false;
  }

  if(valid)
    apply_rules_profile(live, rules_profile);

  live.transposition_table.clear();
  return valid ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_set_position_startpos(void) {
  Engine& live = live_engine();
  live.board.parse_fen(Fen::start_position);
  live.transposition_table.clear();
  return 1;
}

FFI_PLUGIN_EXPORT int mah_apply_move(const char* move) {
  if(move == nullptr || *move == '\0')
    return 0;

  return live_engine().apply_move(move) ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_get_fen(char* out_fen, const int out_len, const int fullmove_number) {
  return copy_out_string(live_engine().board.to_fen(fullmove_number), out_fen, out_len);
}

FFI_PLUGIN_EXPORT int mah_game_status(void) {
  Board& board = live_engine().board;

  if(board.has_legal_move())
    return mah_status_ongoing;

  // a missing king is a lost position rather than undefined behaviour
  const u64 king = board.state.bitboards[board.state.side == white ? K : k];
  if(!king || board.in_check())
    return mah_status_checkmate;
  return mah_status_stalemate;
}

FFI_PLUGIN_EXPORT int mah_best_move_depth(const int depth, char* out_move, const int out_len) {
  Engine& live = live_engine();
  live.time_control.reset();
  return copy_out_move(search_best_move(live, std::max(depth, 1)), out_move, out_len);
}

FFI_PLUGIN_EXPORT int mah_best_move_time(const int movetime_ms, char* out_move, const int out_len) {
  Engine& live = live_engine();
  live.time_control.set_movetime(movetime_ms);
  return copy_out_move(search_best_move(live, Limits::max_ply), out_move, out_len);
}

FFI_PLUGIN_EXPORT int mah_generate_custom_position_fen(const int side_to_move, const unsigned int seed, char* out_fen, const int out_len) {
  if(out_fen == nullptr || out_len <= 0)
    return 0;

  Board& board = live_engine().board;
  if(!generate_custom_position(board, side_to_move, seed))
    return 0;

  return copy_out_string(custom_position_fen(board), out_fen, out_len);
}

FFI_PLUGIN_EXPORT int mah_set_hash_mb(const int mb) {
  live_engine().transposition_table.resize(std::clamp(mb, ffi_min_hash_mb, ffi_max_hash_mb));
  return 1;
}

FFI_PLUGIN_EXPORT int mah_set_threads(const int threads) {
  live_engine().threads = std::clamp(threads, Engine::min_threads, Engine::max_threads);
  return 1;
}

FFI_PLUGIN_EXPORT int mah_set_skill_level(const int skill_level) {
  live_engine().search_config = SearchConfig::for_skill(skill_level);
  return 1;
}

FFI_PLUGIN_EXPORT int mah_set_difficulty_level(const int difficulty_level) {
  live_engine().search_config = SearchConfig::for_difficulty(difficulty_level);
  return 1;
}

FFI_PLUGIN_EXPORT int mah_set_eval_mode(const int eval_mode) {
  live_engine().eval_config.eval_mode = eval_mode == static_cast<int>(EvalMode::nnue) ? EvalMode::nnue : EvalMode::classic;
  return 1;
}

FFI_PLUGIN_EXPORT int mah_load_weights(const char* path) {
  return live_engine().nnue.load_weights(path) ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_load_weights_from_bytes(const unsigned char* bytes, const int len, const char* weights_version_name) {
  Engine& live = live_engine();
  if(len <= 0)
    return 0;

  return live.nnue.load_weights_from_bytes(bytes, static_cast<std::size_t>(len), weights_version_name) ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_load_default_weights(void) {
  return live_engine().nnue.load_default_weights() ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_unload_weights(void) {
  live_engine().nnue.unload_weights();
  return 1;
}

FFI_PLUGIN_EXPORT int mah_get_eval_status(char* out_status, const int out_len) {
  return copy_out_string(eval_status_string(live_engine()), out_status, out_len);
}

FFI_PLUGIN_EXPORT int mah_get_weights_version(char* out_version, const int out_len) {
  return copy_out_string(live_engine().nnue.weights_version(), out_version, out_len);
}

FFI_PLUGIN_EXPORT int mah_get_nnue_info(char* out_info, const int out_len) {
  const Engine& live = live_engine();
  if(out_info == nullptr || out_len <= 0)
    return 0;

  const bool requested = nnue_requested(live);
  const bool weights_loaded = live.nnue.has_loaded_weights();
  const int written = std::snprintf(out_info,
                                    static_cast<std::size_t>(out_len),
                                    "{\"requestedEvalMode\":\"%s\",\"activeEvalMode\":\"%s\",\"status\":\"%s\",\"weightsVersion\":\"%s\","
                                    "\"loadedPath\":\"%s\",\"weightsLoaded\":%d,\"nnueBackendReady\":%d,\"usingClassicFallback\":%d,"
                                    "\"uiDifficulty\":%d,\"skillLevel\":%d,\"accumulatorInitialized\":%d}",
                                    requested ? "nnue" : "classic",
                                    nnue_active(live) ? "nnue" : "classic",
                                    eval_status_string(live),
                                    live.nnue.weights_version().c_str(),
                                    live.nnue.loaded_path().c_str(),
                                    weights_loaded ? 1 : 0,
                                    Nnue::backend_ready() ? 1 : 0,
                                    requested && !weights_loaded ? 1 : 0,
                                    live.search_config.ui_difficulty,
                                    live.search_config.skill_level,
                                    nnue_active(live) ? 1 : 0);
  return written >= 0 && written < out_len ? 1 : 0;
}

FFI_PLUGIN_EXPORT int mah_shutdown(void) {
  engine.reset();
  return 1;
}
