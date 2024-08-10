cmake_minimum_required(VERSION 3.22.2)

project( test_opendialog)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin/${CMAKE_BUILD_TYPE}/)

add_executable(test_opendialog ../../test/test_opendialog.c)
target_link_libraries(test_opendialog nfd)