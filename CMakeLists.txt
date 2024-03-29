cmake_minimum_required(VERSION 3.25)

project(Maharajah VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This project requires a 64-bit compiler.")
endif()

include(FetchContent)

fetchcontent_declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG v1.17.0
)

fetchcontent_makeavailable(googletest)

add_subdirectory(sources)
add_subdirectory(tests)
