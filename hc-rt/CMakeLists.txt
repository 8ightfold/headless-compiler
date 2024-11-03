include_guard(DIRECTORY)

project(
  hcrt-rt
  VERSION 0.1.32
  LANGUAGES CXX
)

include(SetFlags)
include(SetVersion)

set_version(HCRT PROJECT)
set(HC_RSYS "src/Sys/${PLATFORM}")
set(HC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(include)

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
    src/Bootstrap/StringMerger.cpp
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
