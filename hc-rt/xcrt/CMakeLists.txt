include_guard(DIRECTORY)

add_library(hcrt-xinc INTERFACE)
add_library(hcrt::xinc ALIAS hcrt-xinc)
target_include_directories(hcrt-xinc INTERFACE include)
target_forward_options(hcrt-xinc INTERFACE RT_MAX_ATEXIT)
target_compile_definitions(hcrt-xinc INTERFACE __XCRT__=1)
target_compile_options(hcrt-xinc INTERFACE -nostdinc++ -nostdinc -static)
target_link_options(hcrt-xinc INTERFACE -nostdlib -nostartfiles)

if(WIN32)
  target_link_options(hcrt-xinc INTERFACE 
    -Wl,-e,mainCRTStartup 
    -mno-stack-arg-probe
  )
endif()

##########################################################################

add_library(hcrt-xcrt STATIC
  Generic/String/Memcmp.cpp
  Generic/String/Memcpy.cpp
  Generic/String/Memset.cpp
  Generic/String/XStrcmp.cpp
  Generic/String/XStrlen.cpp
  Generic/String/XStrncmp.cpp
)
add_library(hcrt::xcrt ALIAS hcrt-xcrt)

target_link_libraries(hcrt-xcrt 
  PUBLIC
    hcrt::inc hcrt::xinc 
  PRIVATE
    hcrt::src
)
target_include_directories(hcrt-xcrt 
  PUBLIC
    Generic ${HC_PLATFORM_PREFIX}
  PRIVATE
    ${HC_SRC_DIR}
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM})
  add_subdirectory(${PLATFORM})
else()
  message(FATAL_ERROR "The HC runtime is not supported on your platform.")
endif()
