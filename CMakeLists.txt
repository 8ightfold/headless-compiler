cmake_minimum_required(VERSION 3.17)
include_guard(DIRECTORY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

project(
  headless-compiler
  VERSION 0.1.5
  LANGUAGES CXX
)

include(scripts/SetInternalFlag.cmake)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL "Apple Clang is currently unsupported.")
elseif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL "Unsupported compiler!")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(HC_DEBUG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(HC_DEBUG ON)
else()
  set(HC_DEBUG OFF)
endif()

if(HC_DEBUG)
  set(HC_ENABLE_LTO OFF)
endif()

option(HC_CHECK_INVARIANTS "Enable invariant checking." OFF)
# Currently broken :(
option(HC_ENABLE_LTO "Enable interprocedural optimization." OFF)

message("invariant checking: ${HC_ENABLE_LTO}")
message("interprocedural optimization: ${HC_ENABLE_LTO}")

add_executable(driver Driver.cpp 
  src/Common/Memory.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Bootstrap/Win64KernelDefs.cpp
  src/Bootstrap/ModuleParser.cpp
)
target_include_directories(driver PUBLIC include)
target_link_options(driver PUBLIC -Wl,--stack,0xF42400)
set_property(TARGET driver PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})
target_internal_flags(driver PUBLIC HC_DEBUG)
target_internal_flags(driver PUBLIC HC_CHECK_INVARIANTS)
target_internal_flags(driver PUBLIC HC_ENABLE_LTO)
