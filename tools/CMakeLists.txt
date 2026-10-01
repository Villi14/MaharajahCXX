# Native CLI over the C interface (mah_*), port of MaharajahC's maharajah_tool
add_executable(maharajah_tool maharajah_tool.cpp)
target_link_libraries(maharajah_tool PRIVATE ${PROJECT_NAME}STATIC)

# Texel tuning of the evaluation weights (self-play data and the fit)
add_executable(maharajah_texel texel.cpp)
target_link_libraries(maharajah_texel PRIVATE ${PROJECT_NAME}STATIC)

add_test(NAME maharajah_tool_generate COMMAND maharajah_tool generate 1 7)
add_test(NAME maharajah_tool_selfplay COMMAND maharajah_tool selfplay 1 1 20 7)

if(NOT WIN32)
  add_test(NAME maharajah_tool_selfplay_json
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/maharajah_tool_selfplay_json.sh
            $<TARGET_FILE:maharajah_tool>
            ${CMAKE_CURRENT_BINARY_DIR}/maharajah_tool_selfplay.json)
  add_test(NAME maharajah_tool_bench
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/maharajah_tool_bench_smoke.sh
            $<TARGET_FILE:maharajah_tool>)
  add_test(NAME maharajah_tool_uci
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/maharajah_tool_uci_smoke.sh
            $<TARGET_FILE:maharajah_tool>)
endif()
