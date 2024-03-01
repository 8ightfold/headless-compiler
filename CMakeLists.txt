cmake_minimum_required(VERSION 3.18)
include_guard(DIRECTORY)
include(Setup.cmake)

project(
  headless-compiler
  VERSION 0.1.1
  LANGUAGES CXX C
)

include(SetFlags)
include(SetOptions)
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
option(HC_MULTITHREADED "Enable threads (fibers always allowed)." ON)
option(HC_DO_TRACING "Enable tracing in debug mode." OFF)
option(HC_SOFTWARE_PREFETCH "Enable software prefetching." ON)
option(HC_ENABLE_LTO "Enable IPO/LTO." ON)
option(HC_EXTRA_DIAGNOSTICS "Extra Clang messages." OFF)

valued_option(RT_MAX_THREADS "Maximum amount of threads that can be created." 8)
valued_option(RT_MAX_FILES "Maximum amount of files to be opened at once." 16)
valued_option(RT_MAX_PATH "Maximum length a filepath can be." 512)
valued_option(RT_STRICT_MAX_PATH "Maximum length an internal filepath can be." 260)

if(NOT RT_MAX_THREADS)
  set(HC_MULTITHREADED OFF CACHE BOOL "" FORCE)
endif()

message(STATUS "IPO/LTO: ${HC_ENABLE_LTO}")
message(STATUS "Invariant checking: ${HC_CHECK_INVARIANTS}")
message(STATUS "Debug tracing: ${HC_DO_TRACING}")

if(HC_EXTRA_DIAGNOSTICS)
  message(STATUS "Extra diagnostics: ${HC_EXTRA_DIAGNOSTICS}")
endif()

message(STATUS "Multithreading: ${HC_MULTITHREADED}")
if(HC_MULTITHREADED)
  message(STATUS "Max threads: ${RT_MAX_THREADS}")
endif()

message(STATUS "Max files: ${RT_MAX_FILES}")
message(STATUS "Max path: ${RT_MAX_PATH}")

add_subdirectory(hc-rt)
add_executable(driver Driver.cpp)
target_link_libraries(driver PRIVATE hcrt::dev)
target_include_directories(driver PRIVATE hc-rt/src hc-rt/xcrt)

add_executable(nostartup NoStartup.cpp)
target_link_libraries(nostartup PUBLIC hc::rt)
target_link_options(nostartup PUBLIC -v)
