#ifndef MAHARAJAH_FFI_H_
#define MAHARAJAH_FFI_H_

// C interface of the engine for app integrations (e.g. Dart FFI). Not thread-safe:
// all calls share one engine instance.

#if _WIN32
#  define FFI_PLUGIN_EXPORT __declspec(dllexport)
#else
#  define FFI_PLUGIN_EXPORT __attribute__((visibility("default"))) __attribute__((used))
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
  mah_rules_auto = 0,
  mah_rules_standard = 1,
  mah_rules_variant = 2,
};

// Terminal-state classification of the live position for the side to move.
enum {
  mah_status_ongoing = 0, // the side to move has at least one legal move
  mah_status_checkmate = 1, // no legal moves and the king is in check
  mah_status_stalemate = 2, // no legal moves and the king is not in check
};

// Initialize engine tables and set position to startpos.
// Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_init(void);

// Set position from a FEN string. A FEN without exactly one king per side is
// rejected (returns 0) and the engine is left on a safe startpos.
// Returns 1 on success, 0 on failure.
FFI_PLUGIN_EXPORT int mah_set_position_fen(const char* fen);

// Set position from a FEN string with an explicit rules profile.
// rules_profile: 0 = auto, 1 = standard, 2 = variant.
// A FEN without exactly one king per side is rejected (returns 0) and the
// engine is left on a safe startpos.
// Returns 1 on success, 0 on failure.
FFI_PLUGIN_EXPORT int mah_set_position_fen_with_rules(const char* fen, int rules_profile);

// Set position to standard chess startpos.
// Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_position_startpos(void);

// Apply a move in coordinate notation (e.g. "e2e4", "e7e8q").
// Returns 1 if the move was applied, 0 if invalid/illegal.
FFI_PLUGIN_EXPORT int mah_apply_move(const char* move);

// Serialize the current position to a FEN string with the six standard
// fields, field 7 (per-side variant rights, always emitted) and field 8
// (unmoved-pawn squares, emitted only when the position was loaded with
// per-pawn state), so a get_fen -> set_fen round-trip is lossless.
// The engine does not track the fullmove number, so it is supplied by the
// caller via fullmove_number (values below 1 are clamped to 1) and written as
// the sixth FEN field. The other fields are read from live engine state.
// out_len should be at least 128 to hold any legal position (field 8 can add
// up to 16 square names). Returns 1 on success, 0 on failure (bad args /
// buffer too small).
FFI_PLUGIN_EXPORT int mah_get_fen(char* out_fen, int out_len, int fullmove_number);

// Classify the live position for the side to move as a terminal state.
// Returns one of mah_status_ongoing / mah_status_checkmate /
// mah_status_stalemate. A move must have been applied (or a FEN loaded) first;
// the classification is for whichever side is to move in the current position.
FFI_PLUGIN_EXPORT int mah_game_status(void);

// Find best move by depth. Writes move string to out_move.
// out_len must be at least 5 (or 6 for promotions). Returns 1 if a move is
// found.
FFI_PLUGIN_EXPORT int mah_best_move_depth(int depth, char* out_move, int out_len);

// Find best move by movetime in milliseconds. Writes move string to out_move.
// out_len must be at least 5 (or 6 for promotions). Returns 1 if a move is
// found.
FFI_PLUGIN_EXPORT int mah_best_move_time(int movetime_ms, char* out_move, int out_len);

// Generate a reasonable custom-start FEN.
// side_to_move: 0 = white, 1 = black, any other value = random.
// Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_generate_custom_position_fen(int side_to_move, unsigned int seed, char* out_fen, int out_len);

// Resize hash table (MB, clamped to [4, 1024]). Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_hash_mb(int mb);

// Set the number of search threads, clamped to [1, 64]. More than one searches
// faster but no longer returns the same move for the same position every time.
// Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_threads(int threads);

// Set engine skill level in the range [1..10]. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_skill_level(int skill_level);

// Set UI difficulty level in the range [1..5]. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_difficulty_level(int difficulty_level);

// Set evaluation mode: 0 = classic, 1 = nnue (the default, with the built-in
// network). NNUE without loaded weights evaluates classically. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_set_eval_mode(int eval_mode);

// Load NNUE weights from a file path; used once the eval mode is nnue. A file
// that is not a network of this engine's shape is rejected. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_load_weights(const char* path);

// Load NNUE weights from a bytes buffer, as mah_load_weights. Returns 1 on
// success.
FFI_PLUGIN_EXPORT int mah_load_weights_from_bytes(const unsigned char* bytes, int len, const char* weights_version_name);

// Load the network compiled into the engine (loaded at start). Returns 1 on
// success.
FFI_PLUGIN_EXPORT int mah_load_default_weights(void);

// Unload current NNUE weights and reset to fallback state. Returns 1 on
// success.
FFI_PLUGIN_EXPORT int mah_unload_weights(void);

// Get current eval backend status string.
// Values: classic_ready, nnue_ready. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_get_eval_status(char* out_status, int out_len);

// Get current loaded weights version string, or empty if none. Returns 1 on
// success.
FFI_PLUGIN_EXPORT int mah_get_weights_version(char* out_version, int out_len);

// Get a compact JSON description of eval/NNUE runtime state, including
// requested vs active backend. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_get_nnue_info(char* out_info, int out_len);

// Release engine resources. Returns 1 on success.
FFI_PLUGIN_EXPORT int mah_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // MAHARAJAH_FFI_H_
