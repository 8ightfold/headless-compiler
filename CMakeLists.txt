cmake_minimum_required(VERSION 3.17)
include_guard(DIRECTORY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

project(
  headless-compiler
  VERSION 0.1.1
  LANGUAGES CXX C
)

include(SetInternalFlag)
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
option(HC_SOFTWARE_PREFETCH "Enable software prefetching." ON)
option(HC_ENABLE_LTO "Enable IPO/LTO." ON)

message(STATUS "invariant checking: ${HC_ENABLE_LTO}")
message(STATUS "IPO/LTO: ${HC_ENABLE_LTO}")

add_subdirectory(hc-rt)

add_executable(driver Driver.cpp)
target_link_libraries(driver PRIVATE hcrt::dev)
target_include_directories(driver PRIVATE hc-rt/src)
target_compile_definitions(driver PRIVATE -D__HC_INTERNAL__=1)

# add_executable(nostartup NoStartup.cpp)
# target_link_libraries(nostartup PUBLIC hc::runtime)
# target_link_options(nostartup PUBLIC -v)
