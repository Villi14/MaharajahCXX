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
