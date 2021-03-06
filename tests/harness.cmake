if (in)
  execute_process(COMMAND ${process} INPUT_FILE ${in} OUTPUT_FILE ${out})
else()
  execute_process(COMMAND ${process} OUTPUT_FILE ${out})
endif()

if (chk)
  file(READ ${out} txt)
  string(FIND "${txt}" "${chk}" at)
  if (at LESS 0)
    message(FATAL_ERROR "Failed to find ${chk} in output")
  endif ()
endif ()
