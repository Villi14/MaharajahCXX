set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS LIST_DIRECTORIES false *.cpp)

list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

# Browser module (mah_wasm_*) for the web game, built by tools/build_wasm.sh.
# The FEN parser throws, so exceptions are compiled in (native wasm exceptions).
# No -flto: with LTO (emcc 6.0.10) a thrown exception is never caught.
# The search keeps a move list per ply on the stack (MaharajahC uses globals), which
# overflows Emscripten's default 64 KB stack; 1 MB covers max_ply plies with room.
if(EMSCRIPTEN)
  set(MAHARAJAH_WASM_EXPORTS
    _mah_wasm_init _mah_wasm_set_position_fen _mah_wasm_apply_move _mah_wasm_game_status
    _mah_wasm_get_fen _mah_wasm_best_move_depth _mah_wasm_best_move_time)
  list(JOIN MAHARAJAH_WASM_EXPORTS "," MAHARAJAH_WASM_EXPORTS)

  add_executable(maharajah_engine ${ENGINE_SOURCES})
  target_compile_options(maharajah_engine PRIVATE -fwasm-exceptions)
  target_link_options(maharajah_engine PRIVATE
    -fwasm-exceptions
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=67108864
    -sSTACK_SIZE=1048576
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME=createMaharajahEngine
    -sENVIRONMENT=web,node
    -sNO_EXIT_RUNTIME=1
    "-sEXPORTED_FUNCTIONS=[${MAHARAJAH_WASM_EXPORTS}]"
    "-sEXPORTED_RUNTIME_METHODS=[cwrap]")
  set_target_properties(maharajah_engine PROPERTIES SUFFIX ".js")
  return()
endif()

add_library(${BINARY}STATIC STATIC ${ENGINE_SOURCES})

add_executable(${BINARY} main.cpp)
target_link_libraries(${BINARY} PRIVATE ${BINARY}STATIC)

# C interface (mah_*) for app integrations
option(MAHARAJAH_BUILD_FFI "Build the maharajah_ffi shared library" ON)
if(MAHARAJAH_BUILD_FFI)
  add_library(maharajah_ffi SHARED ${ENGINE_SOURCES})
  set_target_properties(maharajah_ffi PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
  if(ANDROID)
    target_link_options(maharajah_ffi PRIVATE "-Wl,-z,max-page-size=16384")
  endif()
endif()
