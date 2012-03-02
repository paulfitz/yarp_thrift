cmake_minimum_required(VERSION 2.6)

project(thrift_compiler)


# should be: ..../thrift/compiler/cpp
find_path(THRIFT_ROOT compiler/cpp/src/thrifty.yy PATHS ${CMAKE_SOURCE_DIR}/thrift ${CMAKE_SOURCE_DIR}/../thrift ENV THRIFT_ROOT DOC "Location of Thrift source code repository")

if (THRIFT_ROOT)
  set(THRIFT_COMPILER_SOURCE ${THRIFT_ROOT}/compiler/cpp)
endif ()

option(WITH_INSERTS "Add extra generators" ON)

file(GLOB_RECURSE thrift_source_cc ${THRIFT_COMPILER_SOURCE}/src/*.cc)
file(GLOB_RECURSE thrift_source_c ${THRIFT_COMPILER_SOURCE}/src/*.c)
if (WITH_INSERTS)
  file(GLOB_RECURSE insert_source_cc inserts/*.cc)
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

include_directories(${THRIFT_COMPILER_SOURCE})
include_directories(${THRIFT_COMPILER_SOURCE}/src)
include_directories(${THRIFT_COMPILER_SOURCE}/src/generate)
BISON_TARGET(tcc_parser ${THRIFT_COMPILER_SOURCE}/src/thrifty.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
FLEX_TARGET(tcc_scanner ${THRIFT_COMPILER_SOURCE}/src/thriftl.ll  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(tcc_scanner tcc_parser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
if (THRIFT_ROOT)
  add_executable(tcc 
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc
    ${thrift_source_cc}
    ${thrift_source_c}
    ${insert_source_cc}
    )
else ()
  message(FATAL_ERROR "Please supply location of Thrift source code (THRIFT_ROOT)")
endif ()
