include_guard(DIRECTORY)

# Applies the function or macro `fn` to each element in a list.
# This will pass the loop element name as well as the element's value.
# eg. fn(var_name ${var_name})
#  _hc_mutate_list(<list> <function|macro>)
function(_hc_mutate_list ls fn)
  set(OUT_LIST "")
  foreach(elem IN LISTS ${ls})
    cmake_language(CALL ${fn} elem ${elem})
    list(APPEND OUT_LIST "${elem}")
  endforeach()
  set(${ls} "${OUT_LIST}" PARENT_SCOPE)
endfunction()

##======================================================================##
## target_internal_flags
##======================================================================##

macro(_hc_make_intflag arg flag)
  if(${${flag}})
    set(${arg} "-D_${flag}=1")
  else()
    if(NOT DEFINED ${flag})
      message(WARNING "`${flag}` is NOT defined! Defaulting to OFF.")
      # TODO: Set in parent scope?
    endif()
    set(${arg} "-D_${flag}=0")
  endif()
endmacro()

macro(_hc_int_flags tgt vis ls)
  if(DEFINED ${ls})
    _hc_mutate_list(${ls} _hc_make_intflag)
    target_compile_definitions(${tgt} ${vis} ${${ls}})
  endif()
endmacro()

# Assign flags to a target. Converts NAME to _NAME.
#  target_internal_flags(<target> <INTERFACE|PUBLIC|PRIVATE> [items...]...)
function(target_internal_flags tgt)
  cmake_parse_arguments(X "" "" 
    "PUBLIC;PRIVATE;INTERFACE" ${ARGN})
  
  get_target_property(type ${tgt} TYPE)
  if(type STREQUAL "INTERFACE_LIBRARY")
    if(DEFINED X_PUBLIC OR DEFINED X_PRIVATE)
      message(SEND_ERROR "target_internal_flags may only "
        "set INTERFACE properties on INTERFACE targets")
      return()
    endif()
  endif()

  _hc_int_flags(${tgt} PUBLIC X_PUBLIC)
  _hc_int_flags(${tgt} PRIVATE X_PRIVATE)
  _hc_int_flags(${tgt} INTERFACE X_INTERFACE)
endfunction()

# Assign flag to a target. Converts NAME to _NAME.
#  target_internal_flags(<target> <INTERFACE|PUBLIC|PRIVATE> [item])
function(target_internal_flags_ tgt vis flag)
  if(NOT DEFINED ${flag})
    message(WARNING "`${flag}` is NOT defined! Defaulting to OFF.")
    set(${flag} OFF PARENT_SCOPE)
  endif()
  if(${flag})
    target_compile_definitions(${tgt} ${vis} "_${flag}=1")
  else()
    target_compile_definitions(${tgt} ${vis} "_${flag}=0")
  endif()
endfunction()

##======================================================================##
## target_forward_options
##======================================================================##

macro(_hc_make_fwdarg arg val)
  set(${arg} "-D${val}=${${val}}")
endmacro()

macro(_hc_fwd_opts tgt vis ls)
  if(DEFINED ${ls})
    _hc_mutate_list(${ls} _hc_make_fwdarg)
    target_compile_definitions(${tgt} ${vis} ${${ls}})
  endif()
endmacro()

# Pass CMake definitions to a target
#  target_forward_options(<target> <INTERFACE|PUBLIC|PRIVATE> [items...]...)
function(target_forward_options tgt)
  cmake_parse_arguments(X "" "" 
    "PUBLIC;PRIVATE;INTERFACE" ${ARGN})
  
  get_target_property(type ${tgt} TYPE)
  if(type STREQUAL "INTERFACE_LIBRARY")
    if(DEFINED X_PUBLIC OR DEFINED X_PRIVATE)
      message(SEND_ERROR "forward_options may only "
        "set INTERFACE properties on INTERFACE targets")
      return()
    endif()
  endif()

  _hc_fwd_opts(${tgt} PUBLIC X_PUBLIC)
  _hc_fwd_opts(${tgt} PRIVATE X_PRIVATE)
  _hc_fwd_opts(${tgt} INTERFACE X_INTERFACE)
endfunction()
