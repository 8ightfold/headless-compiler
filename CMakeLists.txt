cmake_minimum_required(VERSION 3.11)
include_guard(DIRECTORY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

project(
  headless-compiler
  VERSION 0.1.3
  LANGUAGES CXX
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL "Apple Clang is currently unsupported.")
elseif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL "Unsupported compiler!")
endif()

option(HC_CHECK_INVARIANTS "Enable invariant checking." OFF)

add_executable(driver Driver.cpp 
  src/Common/Memory.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Bootstrap/Win64KernelDefs.cpp
)
target_include_directories(driver PUBLIC include)
target_link_options(driver PUBLIC -Wl,--stack,0xF42400)
target_compile_definitions(driver PUBLIC
  $<$<CONFIG:Debug>:_HC_DEBUG>
  $<$<CONFIG:RelWithDebInfo>:_HC_DEBUG>
)
# TODO: Add macro for this?
target_compile_definitions(driver PUBLIC "_HC_CHECK_INVARIANTS=$<BOOL:${HC_CHECK_INVARIANTS}>")
