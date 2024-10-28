include_guard(DIRECTORY)

string(REPLACE "\." "_" HCRT_VERSION_POSTFIX ${PROJECT_VERSION})
configure_file(__config.inc.in
  "${CMAKE_CURRENT_SOURCE_DIR}/__config.inc"
  @ONLY
  NEWLINE_STYLE LF
)

add_library(hcrt-inc INTERFACE)
add_library(hcrt::inc ALIAS hcrt-inc)
target_include_directories(hcrt-inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
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
