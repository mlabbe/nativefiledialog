cmake_minimum_required(VERSION 3.8.0)
project(nativefiledialog)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX 1)
endif()

set(SOURCES src/nfd_common.c)

# Add specific implementations
if (WIN32)
    list(APPEND SOURCES src/nfd_win.cpp)
    list(APPEND PREPROCESSOR_DEFINITIONS 
        UNICODE 
        _UNICODE
    )
elseif (APPLE)
    list(APPEND SOURCES src/nfd_cocoa.m)
elseif (LINUX)
    list(APPEND SOURCES src/nfd_gtk.c)
else()
    message(FATAL_ERROR "Cannot detect your system")
endif()

add_library(nativefiledialog ${SOURCES})
target_include_directories(nativefiledialog PUBLIC src/include)
target_compile_definitions(nativefiledialog PUBLIC ${PREPROCESSOR_DEFINITIONS})

if(LINUX)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(GTK "gtk+-3.0")
        if (GTK_FOUND)    
            target_link_libraries(nativefiledialog ${GTK_LIBRARIES})
            add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
        endif()
    endif()
endif()

install(TARGETS nativefiledialog DESTINATION lib)