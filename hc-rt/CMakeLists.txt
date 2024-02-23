include_guard(DIRECTORY)

project(
  hc-runtime
  VERSION 0.1.16
  LANGUAGES CXX
)

add_library(hcrt-dev STATIC
  src/Common/CheckFundamental.cpp
  src/Common/Memory.cpp
  src/BinaryFormat/MagicMatcher.cpp
  src/Bootstrap/Win64KernelDefs.cpp
  src/Bootstrap/ModuleParser.cpp
  src/Bootstrap/COFFModule.cpp
  src/Bootstrap/StubParser.cpp
  src/Bootstrap/Syscalls.cpp
  src/Sys/IOFile.cpp
)
add_library(hcrt::dev ALIAS hcrt-dev)
target_include_directories(hcrt-dev PUBLIC include PRIVATE src)
target_compile_definitions(hcrt-dev PUBLIC -DHC_MAX_FILE_SLOTS=${HC_MAX_FILE_SLOTS})
target_compile_definitions(hcrt-dev PRIVATE -D__HC_INTERNAL__=1)
target_compile_options(hcrt-dev PUBLIC -fno-exceptions)
target_link_options(hcrt-dev PUBLIC -Wl,--stack,0xF42400 -nostdlib++)
set_property(TARGET hcrt-dev PROPERTY INTERPROCEDURAL_OPTIMIZATION ${HC_ENABLE_LTO})

if(WIN32)
  target_sources(hcrt-dev PUBLIC
    src/Sys/Win/Mutex.cpp
    src/Sys/Win/IOFile.cpp
    src/Sys/Win/PFiles.cpp
  )
else()
  message(FATAL_ERROR "hc is not supported on your platform.")
endif()

include(SetInternalFlag)
target_internal_flags(hcrt-dev PUBLIC HC_DEBUG)
target_internal_flags(hcrt-dev PUBLIC HC_TRUE_DEBUG)
target_internal_flags(hcrt-dev PUBLIC HC_CHECK_INVARIANTS)
target_internal_flags(hcrt-dev PUBLIC HC_SOFTWARE_PREFETCH)
target_internal_flags(hcrt-dev PUBLIC HC_ENABLE_LTO)

# Runtime 

add_library(hc-runtime STATIC
  xcrt/String/Memcpy.cpp
  xcrt/String/Memset.cpp
)
add_library(hc::runtime ALIAS hc-runtime)
target_link_libraries(hc-runtime PUBLIC hcrt-dev)
target_include_directories(hc-runtime PRIVATE src xcrt)
target_compile_definitions(hc-runtime PRIVATE -D__HC_INTERNAL__=1)
target_compile_options(hc-runtime PUBLIC -fno-exceptions -nostdinc++ -nostdinc -static)
target_link_options(hc-runtime PUBLIC -nostdlib++ -nostdlib -nostartfiles)

if(WIN32)
  target_sources(hc-runtime PRIVATE
    xcrt/Phase0/Startup.cpp
    xcrt/Phase0/Xtors.cpp
    xcrt/Phase0/InitCookie.cpp
    xcrt/Phase1/Initialization.cpp
    xcrt/Phase1/StdIO.cpp
    xcrt/Phase1/Emutils.cpp
    xcrt/Mingw/NtdllLdr.cpp
  )
  target_link_options(hc-runtime PUBLIC -Wl,-e,mainCRTStartup)
else()
  message(FATAL_ERROR "The hc runtime is not supported on your platform.")
endif()
