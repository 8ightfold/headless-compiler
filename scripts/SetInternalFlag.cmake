include_guard(DIRECTORY)

macro(target_internal_flags tgt vis flag)
  if(${flag})
    target_compile_definitions(${tgt} ${vis} "_${flag}=1")
  else()
    target_compile_definitions(${tgt} ${vis} "_${flag}=0")
  endif()
endmacro()
