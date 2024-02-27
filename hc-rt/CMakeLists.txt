include_guard(DIRECTORY)

project(
  hcrt-crt
  VERSION 0.1.19
  LANGUAGES CXX
)

add_library(hcrt-dev STATIC
  src/Common/CheckFundamental.cpp
  src/Common/Memory.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Meta/ID.cpp
  src/Sys/IOFile.cpp
)
add_library(hcrt::dev ALIAS hcrt-dev)
target_include_directories(hcrt-dev PUBLIC include PRIVATE src)
target_forward_options(hcrt-dev PUBLIC RT_MAX_FILES RT_MAX_PATH RT_STRICT_MAX_PATH)
target_compile_definitions(hcrt-dev PRIVATE __HC_INTERNAL__=1)
target_compile_options(hcrt-dev PUBLIC -fno-exceptions PRIVATE -Wall)
target_link_options(hcrt-dev PUBLIC -Wl,--stack,0xF42400 -nostdlib++)
set_property(TARGET hcrt-dev PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})

include(SetFlags)
target_internal_flags(hcrt-dev PUBLIC HC_DEBUG)
target_internal_flags(hcrt-dev PUBLIC HC_TRUE_DEBUG)
target_internal_flags(hcrt-dev PUBLIC HC_CHECK_INVARIANTS)
target_internal_flags(hcrt-dev PUBLIC HC_DO_TRACING)
target_internal_flags(hcrt-dev PUBLIC HC_MULTITHREADED)
target_internal_flags(hcrt-dev PUBLIC HC_SOFTWARE_PREFETCH)
target_internal_flags(hcrt-dev PUBLIC HC_ENABLE_LTO)

if(HC_MULTITHREADED)
  target_forward_options(hcrt-dev PUBLIC RT_MAX_THREADS)
else()
  target_compile_definitions(hcrt-dev PUBLIC RT_MAX_THREADS=0)
endif()

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
    src/Sys/Win/PFiles.cpp
    src/Sys/Win/PathNormalizer.cpp
  )
else()
  message(FATAL_ERROR "hc is not supported on your platform.")
endif()

# xcrt 

add_library(hcrt-crt STATIC
  xcrt/String/Memcpy.cpp
  xcrt/String/Memset.cpp
  xcrt/String/XStrlen.cpp
)
add_library(hc::runtime ALIAS hcrt-crt)
target_link_libraries(hcrt-crt PUBLIC hcrt-dev)
target_include_directories(hcrt-crt PRIVATE src xcrt)
target_compile_definitions(hcrt-crt PRIVATE __HC_INTERNAL__=1)
target_compile_options(hcrt-crt PUBLIC -fno-exceptions -nostdinc++ -nostdinc -static)
target_link_options(hcrt-crt PUBLIC -nostdlib++ -nostdlib -nostartfiles)

if(WIN32)
  target_sources(hcrt-crt PRIVATE
    xcrt/Phase0/Startup.cpp
    xcrt/Phase0/Xtors.cpp
    xcrt/Phase0/InitCookie.cpp
    xcrt/Phase1/Initialization.cpp
    xcrt/Phase1/StdIO.cpp
    xcrt/Phase1/Emutls.cpp
    xcrt/Mingw/NtdllLdr.cpp
  )
  target_link_options(hcrt-crt PUBLIC -Wl,-e,mainCRTStartup)
else()
  message(FATAL_ERROR "The hc runtime is not supported on your platform.")
endif()
