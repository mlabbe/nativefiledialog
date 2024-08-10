cmake_minimum_required(VERSION 3.22.2)

project( test_pickfolder)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin/${CMAKE_BUILD_TYPE}/)

add_executable(test_pickfolder ../../test/test_pickfolder.c)
target_link_libraries(test_pickfolder nfd)