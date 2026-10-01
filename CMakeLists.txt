cmake_minimum_required(VERSION 3.25)

project(Maharajah VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MAHARAJAH_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(MAHARAJAH_SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

enable_testing()

# reported in the UCI "id name" line of the engine and of maharajah_tool
add_compile_definitions(MAHARAJAH_VERSION="${PROJECT_VERSION}")

if (MSVC)
  add_compile_options(/W4)
  # standard fopen/setbuf etc. are portable; MSVC's "unsafe" nag isn't useful here
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Emscripten targets wasm32; the engine keeps its bitboards in u64 and does not
# depend on the pointer size
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT EMSCRIPTEN)
  message(FATAL_ERROR "This project requires a 64-bit compiler.")
endif()

# the browser build (tools/build_wasm.sh) needs only the engine module
if (EMSCRIPTEN)
  add_subdirectory(sources)
  return()
endif()

# off for builds that only need the engine (e.g. the server's libmaharajah_ffi):
# skips fetching googletest and building the tests and tools
option(MAHARAJAH_BUILD_TESTS "Build the tests and the command-line tools" ON)

add_subdirectory(sources)

if (NOT MAHARAJAH_BUILD_TESTS)
  return()
endif()

include(FetchContent)

fetchcontent_declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG v1.17.0
)

fetchcontent_makeavailable(googletest)

add_subdirectory(tests)
add_subdirectory(tools)
