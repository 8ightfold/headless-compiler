cmake_minimum_required(VERSION 3.17)
include_guard(DIRECTORY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

project(
  headless-compiler
  VERSION 0.1.12
  LANGUAGES CXX C
)

include(scripts/SetInternalFlag.cmake)
include(CheckIPOSupported)
check_ipo_supported(RESULT hc_ipo_supported LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL "Apple Clang is currently unsupported.")
elseif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL "Unsupported compiler!")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(HC_DEBUG ON)
  set(HC_TRUE_DEBUG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(HC_DEBUG ON)
  set(HC_TRUE_DEBUG OFF)
else()
  set(HC_DEBUG OFF)
  set(HC_TRUE_DEBUG OFF)
endif()

if(HC_DEBUG)
  set(HC_ENABLE_LTO OFF)
elseif(NOT hc_ipo_supported) # Currently broken on Windows :(
  message(STATUS "IPO not supported!")
  set(HC_ENABLE_LTO OFF)
endif()

option(HC_CHECK_INVARIANTS "Enable invariant checking." OFF)
option(HC_ENABLE_LTO "Enable IPO/LTO." ON)

message(STATUS "invariant checking: ${HC_ENABLE_LTO}")
message(STATUS "IPO/LTO: ${HC_ENABLE_LTO}")

add_executable(driver Driver.cpp
  hc-rt/src/Common/Memory.cpp
  hc-rt/src/BinaryFormat/MagicMatcher.cpp
  hc-rt/src/Bootstrap/Win64KernelDefs.cpp
  hc-rt/src/Bootstrap/ModuleParser.cpp
  hc-rt/src/Bootstrap/COFFModule.cpp
  hc-rt/src/Bootstrap/StubParser.cpp
  hc-rt/src/Bootstrap/Syscalls.cpp
)
target_include_directories(driver PUBLIC hc-rt/include PRIVATE hc-rt/src)
target_compile_definitions(driver PRIVATE -D__HC_INTERNAL__=1)
# target_compile_options(driver PUBLIC -nostdinc++)
target_compile_options(driver PUBLIC -fno-exceptions)
target_link_options(driver PUBLIC -Wl,--stack,0xF42400 -nostdlib++)
set_property(TARGET driver PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})
target_internal_flags(driver PUBLIC HC_DEBUG)
target_internal_flags(driver PUBLIC HC_TRUE_DEBUG)
target_internal_flags(driver PUBLIC HC_CHECK_INVARIANTS)
target_internal_flags(driver PUBLIC HC_ENABLE_LTO)

add_executable(nostartup NoStartup.cpp hc-rt/startup/GccMain.cpp)
target_compile_options(nostartup PUBLIC -fno-exceptions -nostdinc++)
target_link_options(nostartup PUBLIC -v -nostdlib++ -nostartfiles)