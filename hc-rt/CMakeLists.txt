include_guard(DIRECTORY)

project(
  hcrt-xcrt
  VERSION 0.1.20
  LANGUAGES CXX
)

##======================================================================##
## Inc - Public Interface
##======================================================================##

add_library(hcrt-inc INTERFACE)
add_library(hcrt::inc ALIAS hcrt-inc)
target_include_directories(hcrt-inc INTERFACE include)
target_forward_options(hcrt-inc INTERFACE RT_MAX_FILES RT_MAX_PATH RT_STRICT_MAX_PATH)
target_compile_options(hcrt-inc INTERFACE -march=native)
target_compile_options(hcrt-inc INTERFACE -fno-exceptions -fdiagnostics-show-template-tree)
target_link_options(hcrt-inc INTERFACE -Wl,--stack,0xF42400 -nostdlib++)
set_property(TARGET hcrt-inc PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})

if(HC_MULTITHREADED)
  target_forward_options(hcrt-inc INTERFACE RT_MAX_THREADS)
else()
  target_compile_definitions(hcrt-inc INTERFACE RT_MAX_THREADS=0)
  target_compile_options(hcrt-inc INTERFACE -fno-threadsafe-statics)
endif()

if(HC_EXTRA_DIAGNOSTICS)
  target_compile_options(hcrt-inc 
    INTERFACE
    -fdiagnostics-show-category=name
    -ggdb
  )
endif()

include(SetFlags)
target_internal_flags(hcrt-inc INTERFACE HC_DEBUG)
target_internal_flags(hcrt-inc INTERFACE HC_TRUE_DEBUG)
target_internal_flags(hcrt-inc INTERFACE HC_CHECK_INVARIANTS)
target_internal_flags(hcrt-inc INTERFACE HC_DO_TRACING)
target_internal_flags(hcrt-inc INTERFACE HC_MULTITHREADED)
target_internal_flags(hcrt-inc INTERFACE HC_SOFTWARE_PREFETCH)
target_internal_flags(hcrt-inc INTERFACE HC_ENABLE_LTO)

##======================================================================##
## Dev - Exposed internals
##======================================================================##

add_library(hcrt-dev STATIC
  src/Common/CheckFundamental.cpp
  src/Common/Memory.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Meta/ID.cpp
  src/Sys/IOFile.cpp
)
add_library(hcrt::dev ALIAS hcrt-dev)
target_link_libraries(hcrt-dev PUBLIC hcrt::inc)
target_include_directories(hcrt-dev PUBLIC include src)
target_compile_definitions(hcrt-dev PUBLIC __HC_INTERNAL__=1)
target_compile_options(hcrt-dev PRIVATE -Wall)

if(WIN32)
  target_sources(hcrt-dev PUBLIC
    src/Bootstrap/Win64KernelDefs.cpp
    src/Bootstrap/ModuleParser.cpp
    src/Bootstrap/COFFModule.cpp
    src/Bootstrap/StubParser.cpp
    src/Bootstrap/Syscalls.cpp
    src/Sys/Core/Nt/CheckPacking.cpp
    src/Sys/Win/Args.cpp
    src/Sys/Win/Mutex.cpp
    src/Sys/Win/IOFile.cpp
    src/Sys/Win/OpaqueError.cpp
    src/Sys/Win/PlatformStatus.cpp
    src/Sys/Win/PFiles.cpp
    src/Sys/Win/PathNormalizer.cpp
  )
else()
  message(FATAL_ERROR "hc is not supported on your platform.")
endif()

##======================================================================##
## xcrt - CRT Implementation
##======================================================================##

add_library(hcrt-xinc INTERFACE)
add_library(hcrt::xinc ALIAS hcrt-xinc)
target_compile_options(hcrt-xinc INTERFACE -fno-exceptions -nostdinc++ -nostdinc -static)
target_link_options(hcrt-xinc INTERFACE -nostdlib++ -nostdlib -nostartfiles)

add_library(hcrt-xcrt STATIC
  xcrt/String/Memcpy.cpp
  xcrt/String/Memset.cpp
  xcrt/String/XStrcmp.cpp
  xcrt/String/XStrlen.cpp
  xcrt/String/XStrncmp.cpp
)
add_library(hcrt::xcrt ALIAS hcrt-xcrt)
target_link_libraries(hcrt-xcrt PUBLIC hcrt::xinc PRIVATE hcrt::dev)
target_include_directories(hcrt-xcrt PUBLIC src xcrt)
target_compile_definitions(hcrt-xcrt PRIVATE __HC_INTERNAL__=1)

if(WIN32)
  target_sources(hcrt-xcrt PUBLIC
    xcrt/Phase0/Startup.cpp
    xcrt/Phase0/Xtors.cpp
    xcrt/Phase0/InitCookie.cpp
    xcrt/Phase1/Initialization.cpp
    xcrt/Phase1/StdIO.cpp
    xcrt/Phase1/Emutls.cpp
    xcrt/Mingw/NtdllLdr.cpp
  )
  target_link_options(hcrt-xinc INTERFACE -Wl,-e,mainCRTStartup)
else()
  message(FATAL_ERROR "The hc runtime is not supported on your platform.")
endif()

##======================================================================##
## Rt - Full User Runtime
##======================================================================##

add_library(hc-rt STATIC xcrt/Stub.cpp)
add_library(hc::rt ALIAS hc-rt)
target_link_libraries(hc-rt 
  PUBLIC hcrt::inc hcrt::xinc 
  PRIVATE hcrt::xcrt
)
