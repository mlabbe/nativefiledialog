cmake_minimum_required(VERSION 3.8.0)
project(nativefiledialog)


set(SOURCES src/nfd_common.c)

# Add specific implementations
if (WIN32)
    list(APPEND SOURCES src/nfd_win.cpp)
elseif (APPLE)
    list(APPEND SOURCES src/nfd_cocoa.m)
elseif (UNIX)
    list(APPEND SOURCES src/nfd_gtk.c)
else()
    message(FATAL_ERROR "Cannot detect your system")
endif()

add_library(nativefiledialog ${SOURCES})
target_include_directories(nativefiledialog PUBLIC src/include)
install(TARGETS nativefiledialog DESTINATION lib)