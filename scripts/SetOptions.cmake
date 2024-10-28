include_guard(DIRECTORY)

# Works like `option(...)`, except with values.
#  valued_option(<variable> "<help_text>" <value>)
macro(valued_option opt msg value)
  set(_HC_VOPT_MSG "${msg}")
  if(NOT DEFINED ${opt})
    set(${opt} ${value} CACHE INTERNAL ${msg})
  elseif(_HC_VOPT_MSG STREQUAL "")
    set(${opt} ${${opt}} CACHE INTERNAL " ")
  else()
    set(${opt} ${${opt}} CACHE INTERNAL ${_HC_VOPT_MSG})
  endif()
endmacro()
