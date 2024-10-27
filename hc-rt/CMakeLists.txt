include_guard(DIRECTORY)

project(
  hcrt-xcrt
  VERSION 0.1.29
  LANGUAGES CXX
)

set(HC_RSYS "src/Sys/${PLATFORM}")
set(HC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

##======================================================================##
## Inc - Public Interface
##======================================================================##

add_library(hcrt-inc INTERFACE)
add_library(hcrt::inc ALIAS hcrt-inc)
target_include_directories(hcrt-inc INTERFACE include)
target_forward_options(hcrt-inc INTERFACE RT_MAX_FILES RT_MAX_PATH RT_STRICT_MAX_PATH)
target_compile_options(hcrt-inc INTERFACE -march=native -ggdb)
target_compile_options(hcrt-inc INTERFACE -Wno-trigraphs -fdiagnostics-show-template-tree)
target_link_options(hcrt-inc INTERFACE -Wl,--stack,0x1000000 -nostdlib++)
set_property(TARGET hcrt-inc PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})

if(NOT HC_RTTI)
  target_compile_options(hcrt-inc INTERFACE -fno-rtti)
endif()

if(NOT HC_EXCEPTIONS)
  target_compile_options(hcrt-inc INTERFACE -fno-exceptions)
  if(NOT HC_DEBUG)
    target_compile_options(hcrt-inc INTERFACE
      -fno-asynchronous-unwind-tables)
  endif()
endif()

if(HC_MULTITHREADED)
  target_forward_options(hcrt-inc INTERFACE RT_MAX_THREADS)
else()
  target_compile_definitions(hcrt-inc INTERFACE RT_MAX_THREADS=0)
  target_compile_options(hcrt-inc INTERFACE -fno-threadsafe-statics)
endif()

if(HC_EMUTLS)
  target_compile_options(hcrt-inc INTERFACE -femulated-tls)
else()
  target_compile_options(hcrt-inc INTERFACE -fno-emulated-tls)
endif()

if(HC_EXTRA_DIAGNOSTICS)
  target_compile_options(hcrt-inc INTERFACE
    -fdiagnostics-show-category=name
  )
endif()

include(SetFlags)
target_internal_flags(
  hcrt-inc INTERFACE
  HC_DEBUG 
  HC_TRUE_DEBUG
  HC_CHECK_INVARIANTS
  HC_DO_TRACING 
  HC_RTTI
  HC_MULTITHREADED
  HC_EMUTLS
  HC_SOFTWARE_PREFETCH
  HC_COMMON_INLINE
  HC_ENABLE_LTO
)

##======================================================================##
## Src - Private Sources
##======================================================================##

add_library(hcrt-src INTERFACE)
target_sources(hcrt-src INTERFACE
  src/Common/CheckFundamental.cpp
  src/Common/Memory.cpp
  src/Common/StrRef.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Meta/ID.cpp
  src/Parcel/StringTable.cpp
  src/Sys/IOFile.cpp
  # Platform Specific
  ${HC_RSYS}/Args.cpp
  ${HC_RSYS}/IOFile.cpp
  ${HC_RSYS}/OpaqueError.cpp
  ${HC_RSYS}/OSMutex.cpp
  ${HC_RSYS}/PlatformStatus.cpp
  ${HC_RSYS}/PFiles.cpp
  ${HC_RSYS}/Shutdown.cpp
)
add_library(hcrt::src ALIAS hcrt-src)
target_link_libraries(hcrt-src INTERFACE hcrt::inc)
target_include_directories(hcrt-src INTERFACE include src)
target_compile_definitions(hcrt-src INTERFACE __HC_INTERNAL__=1)
target_internal_flags(
  hcrt-src INTERFACE 
  HC_FAST_STRING_TABLE
)

if(WIN32)
  target_sources(hcrt-src INTERFACE
    src/Bootstrap/COFFModule.cpp
    src/Bootstrap/ModuleParser.cpp
    src/Bootstrap/StubParser.cpp
    src/Bootstrap/Syscalls.cpp
    src/Bootstrap/UnicodeString.cpp
    src/Bootstrap/Win64KernelDefs.cpp
    src/Bootstrap/CheckPacking.cpp
    ${HC_RSYS}/PathNormalizer.cpp
    ${HC_RSYS}/PathPredictor.cpp
    ${HC_RSYS}/Nt/Except.cpp
    ${HC_RSYS}/Nt/CheckPacking.cpp
  )
endif()

##======================================================================##
## Dev - Exposed internals (with default CRT)
##======================================================================##

add_library(hcrt-dev STATIC xcrt/Stub.cpp)
add_library(hcrt::dev ALIAS hcrt-dev)
target_compile_options(hcrt-dev PRIVATE -Wall -Wno-trigraphs)
target_include_directories(hcrt-dev PUBLIC include src)
target_link_libraries(hcrt-dev 
  PUBLIC  hcrt-inc
  PRIVATE hcrt-src
)

##======================================================================##
## Rt - Full User Runtime
##======================================================================##

add_subdirectory(xcrt) # hcrt::{xinc, xcrt}

add_library(hc-rt STATIC xcrt/Stub.cpp)
add_library(hc::rt ALIAS hc-rt)
target_link_libraries(hc-rt 
  PUBLIC hcrt::inc hcrt::xinc 
  PRIVATE hcrt::xcrt
)
