cmake_minimum_required(VERSION 3.22.2)

project( test_opendialogmultiple)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin/${CMAKE_BUILD_TYPE}/)

add_executable(test_opendialogmultiple ../../test/test_opendialogmultiple.c)
target_link_libraries(test_opendialogmultiple nfd)