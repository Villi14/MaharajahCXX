set(BINARY ${CMAKE_PROJECT_NAME}TESTS)

if (MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(${BINARY} ${TEST_SOURCES})

target_include_directories(${BINARY}
  PRIVATE
  headers
)

target_link_libraries(${BINARY}
  PRIVATE
  ${PROJECT_NAME}STATIC
  GTest::gmock
  GTest::gtest_main
)

target_compile_definitions(${BINARY}
  PRIVATE
  MAHARAJAH_TESTING
)

include(GoogleTest)
gtest_discover_tests(${BINARY})

# UCI smoke test of the engine binary, from MaharajahC
if(NOT WIN32)
  add_test(NAME uci_smoke
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/uci_smoke.sh $<TARGET_FILE:${PROJECT_NAME}>)
endif()
