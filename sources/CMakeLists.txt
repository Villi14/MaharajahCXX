set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS LIST_DIRECTORIES false *.cpp)

list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

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
