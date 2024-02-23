include_guard(DIRECTORY)

macro(valued_option opt msg value)
  if(NOT DEFINED ${opt})
    set(${opt} ${value} CACHE INTERNAL ${msg})
  else()
    set(${opt} "${${opt}}" CACHE INTERNAL ${msg})
  endif()
endmacro()
