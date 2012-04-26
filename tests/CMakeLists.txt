
set(TESTS ${CMAKE_SOURCE_DIR}/tests)
get_target_property(tcc tcc LOCATION)

macro(add_out_test name out chk)
  add_test(${name} ${CMAKE_COMMAND} -Dname=${name} "-Dout:STRING=${out}" "-Dchk:STRING=${chk}" "-Dprocess:STRING=${ARGN}" -P ${CMAKE_CURRENT_SOURCE_DIR}/harness.cmake)
endmacro()

##########################################################################
##########################################################################
#
# Check for runnability
#
##########################################################################
##########################################################################

if (WITH_BASE)
  add_test(minimal_cpp ${tcc} --gen cpp ${TESTS}/minimal/test.thrift)
endif ()

add_test(minimal_yarp ${tcc} --gen yarp ${TESTS}/minimal/test.thrift)

# Check for failure for non-existent generator type.
# Complicated a big because the compiler doesn't return an error code,
# just a printed message

add_out_test(minimal_nonexistent minimal_nonexistent.out 
  "Unable to get a generator"
  ${tcc} --gen nonexistent ${TESTS}/minimal/test.thrift)

add_out_test(minimal_existent minimal_existent.out 
  "Unable to get a generator"
  ${tcc} --gen yarp ${TESTS}/minimal/test.thrift)
set_property(TEST minimal_existent PROPERTY WILL_FAIL TRUE)




##########################################################################
##########################################################################
#
# Check for compilability
#
##########################################################################
##########################################################################

