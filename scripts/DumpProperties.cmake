include_guard(DIRECTORY)

if(NOT DEFINED ENV{CMAKE_PROPERTY_LIST})
  execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
  STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
  STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
  list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)
  
  set(ENV{CMAKE_PROPERTY_LIST} "${CMAKE_PROPERTY_LIST}")
  unset(CMAKE_PROPERTY_LIST)
endif()

# Lists all the properties of a given target.
# ALIAS targets will get resolved to the target they alias.
#  dump_target_properties(<target>)
function(dump_target_properties tgt)
  if(NOT TARGET ${tgt})
    message(STATUS "No target named `${tgt}`.")
    return()
  endif()

  get_target_property(tgt_alias ${tgt} ALIASED_TARGET)
  if("${tgt_alias}" STREQUAL "tgt_alias-NOTFOUND")
    set(tgt_name ${tgt})
  else()
    set(tgt_name ${tgt_alias})
  endif()

  foreach(prop $ENV{CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
    get_target_property(propval ${tgt_name} ${prop})
    if (propval)
      message("[${tgt}] ${prop}: ${propval}")
    endif()
  endforeach()
endfunction()
