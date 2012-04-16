
set(ODIR ${CMAKE_BINARY_DIR}/gen)

get_target_property(TCC_LOC tcc LOCATION)

add_custom_command(OUTPUT ${ODIR}/Demo.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ODIR}
  COMMAND ${TCC_LOC} -out ${ODIR} --gen yarp 
  ${CMAKE_CURRENT_SOURCE_DIR}/demo.thrift
  DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/demo.thrift
)

add_custom_command(OUTPUT ${ODIR}/Motor.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ODIR}
  COMMAND ${TCC_LOC} -out ${ODIR} --gen yarp 
  ${CMAKE_CURRENT_SOURCE_DIR}/motor.thrift
  DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/motor.thrift
)

add_custom_command(OUTPUT ${ODIR}/World.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ODIR}
  COMMAND ${TCC_LOC} -out ${ODIR} --gen yarp 
  ${CMAKE_CURRENT_SOURCE_DIR}/world.thrift
  DEPENDS tcc ${CMAKE_CURRENT_SOURCE_DIR}/world.thrift
)

find_package(YARP REQUIRED)
include_directories(${ODIR})
include_directories(${YARP_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/yarp/include)

add_executable(test_demo ${ODIR}/Demo.h demo.cpp 
  ${CMAKE_SOURCE_DIR}/yarp/include/yarp/os/Wire.h)
target_link_libraries(test_demo ${YARP_LIBRARIES})

add_executable(motor_client ${ODIR}/Motor.h motor_client.cpp 
  ${CMAKE_SOURCE_DIR}/yarp/include/yarp/os/Wire.h)
target_link_libraries(motor_client ${YARP_LIBRARIES})

add_executable(motor_server ${ODIR}/Motor.h motor_server.cpp 
  ${CMAKE_SOURCE_DIR}/yarp/include/yarp/os/Wire.h)
target_link_libraries(motor_server ${YARP_LIBRARIES})

add_executable(world_client ${ODIR}/World.h world_client.cpp 
  ${CMAKE_SOURCE_DIR}/yarp/include/yarp/os/Wire.h)
target_link_libraries(world_client ${YARP_LIBRARIES})

