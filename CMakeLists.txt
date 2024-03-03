cmake_minimum_required(VERSION 3.13)
project(MaharajahCPP VERSION 0.1.0)

include_directories(src)

add_subdirectory(src)
add_subdirectory(tst)

include(FetchContent)
FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest
	GIT_TAG v1.14.0)
FetchContent_GetProperties(googletest)
FetchContent_Populate(googletest)
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
