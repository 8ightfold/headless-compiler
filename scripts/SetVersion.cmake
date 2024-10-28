include_guard(DIRECTORY)
include(ListUtils)

macro(_hc_set_ver ls idx item in out)
  if(DEFINED ${in}_VERSION_${item})
    set(_VERSION_${item} "${${in}_VERSION_${item}}")
    if(_VERSION_${item} STREQUAL "")
      set(_VERSION_${item} 0)
    endif()
  else()
    list_get_or(${ls} ${idx} 0 _VERSION_${item})
  endif()

  # message(STATUS "${out}_VERSION_${item}: ${_VERSION_${item}}")
  set(${out}_VERSION_${item} "${_VERSION_${item}}" PARENT_SCOPE)
  unset(_VERSION_${item})
endmacro()

function(set_version prefix)
  cmake_parse_arguments(
    PARSE_ARGV 0 _V
    "PROJECT" # Options
    "PREFIX;STRING" # Single value
    "" # Multi value
  )

  if(DEFINED _V_STRING)
    set(_PREFIX _INTERNAL)
    set(_INTERNAL_VERSION ${_V_STRING})
  elseif(_V_PROJECT OR NOT DEFINED _V_PREFIX)
    set(_PREFIX PROJECT)
  else()
    set(_PREFIX ${_V_PREFIX})
  endif()

  if(NOT DEFINED ${_PREFIX}_VERSION)
    message(FATAL_ERROR "'${_PREFIX}_VERSION' must be defined!")
  endif()

  set(${prefix}_VERSION ${${_PREFIX}_VERSION} PARENT_SCOPE)
  string(REGEX MATCHALL "[0-9]+" SPLIT_IN_VER ${${_PREFIX}_VERSION})
  
  _hc_set_ver(SPLIT_IN_VER 0 MAJOR ${_PREFIX} ${prefix})
  _hc_set_ver(SPLIT_IN_VER 1 MINOR ${_PREFIX} ${prefix})
  _hc_set_ver(SPLIT_IN_VER 2 PATCH ${_PREFIX} ${prefix})
  _hc_set_ver(SPLIT_IN_VER 3 TWEAK ${_PREFIX} ${prefix})
endfunction(set_version)
