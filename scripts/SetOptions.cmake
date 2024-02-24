include_guard(DIRECTORY)

# Works like `option(...)`, except with values.
#  valued_option(<variable> "<help_text>" <value>)
macro(valued_option opt msg value)
  if(NOT DEFINED ${opt})
    set(${opt} ${value} CACHE INTERNAL ${msg})
  else()
    set(${opt} "${${opt}}" CACHE INTERNAL ${msg})
  endif()
endmacro()
