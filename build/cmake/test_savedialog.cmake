cmake_minimum_required(VERSION 3.22.2)

project( test_savedialog)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin/${CMAKE_BUILD_TYPE}/)

add_executable(test_savedialog ../../test/test_savedialog.c)
target_link_libraries(test_savedialog nfd)