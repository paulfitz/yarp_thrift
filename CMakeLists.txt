cmake_minimum_required(VERSION 2.6)

project(thrift_cmake)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
add_subdirectory(compiler)
add_subdirectory(demo)

enable_testing()
add_subdirectory(tests)
