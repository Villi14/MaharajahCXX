#ifndef MAHARAJAH_WASM_H_
#define MAHARAJAH_WASM_H_

// Browser adapters over the C interface (mah_*), exported by the Emscripten build.
// They return strings instead of filling caller-owned buffers, so JavaScript can read
// them through cwrap. A returned string stays valid until the next call of the same
// kind; failure returns "". Single-threaded, like the rest of the C interface.

#include "../ffi/maharajah_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_PLUGIN_EXPORT int mah_wasm_init(void);
FFI_PLUGIN_EXPORT int mah_wasm_set_position_fen(const char* fen);
FFI_PLUGIN_EXPORT int mah_wasm_apply_move(const char* move);
FFI_PLUGIN_EXPORT int mah_wasm_game_status(void);
FFI_PLUGIN_EXPORT const char* mah_wasm_get_fen(int fullmove_number);
FFI_PLUGIN_EXPORT const char* mah_wasm_best_move_depth(int depth);
FFI_PLUGIN_EXPORT const char* mah_wasm_best_move_time(int movetime_ms);

#ifdef __cplusplus
}
#endif

#endif // MAHARAJAH_WASM_H_
