cmake_minimum_required(VERSION 3.22.2)

project(nfd_build)


message("Building For ${CMAKE_SYSTEM_NAME}")


include(nfd.cmake)
include(test_opendialog.cmake)
include(test_opendialogmultiple.cmake)
include(test_pickfolder.cmake)
include(test_savedialog.cmake)
