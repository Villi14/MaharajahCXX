#include "../../headers/wasm/maharajah_wasm.h"

namespace {

char fen_buffer[256];
char move_buffer[8];

} // namespace

FFI_PLUGIN_EXPORT int mah_wasm_init(void) {
  return mah_init();
}

FFI_PLUGIN_EXPORT int mah_wasm_set_position_fen(const char* fen) {
  return mah_set_position_fen(fen);
}

FFI_PLUGIN_EXPORT int mah_wasm_apply_move(const char* move) {
  return mah_apply_move(move);
}

FFI_PLUGIN_EXPORT int mah_wasm_game_status(void) {
  return mah_game_status();
}

FFI_PLUGIN_EXPORT const char* mah_wasm_get_fen(const int fullmove_number) {
  if(!mah_get_fen(fen_buffer, static_cast<int>(sizeof(fen_buffer)), fullmove_number))
    return "";
  return fen_buffer;
}

FFI_PLUGIN_EXPORT const char* mah_wasm_best_move_depth(const int depth) {
  if(!mah_best_move_depth(depth, move_buffer, static_cast<int>(sizeof(move_buffer))))
    return "";
  return move_buffer;
}

FFI_PLUGIN_EXPORT const char* mah_wasm_best_move_time(const int movetime_ms) {
  if(!mah_best_move_time(movetime_ms, move_buffer, static_cast<int>(sizeof(move_buffer))))
    return "";
  return move_buffer;
}
