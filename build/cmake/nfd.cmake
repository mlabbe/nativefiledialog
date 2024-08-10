cmake_minimum_required(VERSION 3.10)

# Project declaration
project(nfd)

set(SOURCE_PATH ../../src)
# Set output directories for the build types
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../../lib/${CMAKE_BUILD_TYPE}/)

# Set source files

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(SOURCE_FILES
        ${SOURCE_PATH}/nfd_win.cpp
        ${SOURCE_PATH}/nfd_common.c
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Use Zenity: ${USE_ZENITY}")
    if(USE_ZENITY)
        set(SOURCE_FILES
            ../../src/nfd_zenity.c
            ../../src/nfd_common.c
        )
    else()
        set(SOURCE_FILES
            ../../src/nfd_gtk.c
            ../../src/nfd_common.c
        )
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS is detected as "Darwin"
    set(SOURCE_FILES
        ../../src/nfd_cocoa.m   # Objective-C file for Cocoa
        ../../src/nfd_common.c
    )
else()
    message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}")
endif()
# Include directories
include_directories( ${SOURCE_PATH}/include)


# Define the library target
add_library(nfd STATIC ${SOURCE_FILES})
