cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(xdelta)

add_library(xdelta SHARED
    xdelta3.c
    zenfs_xdelta3.cc
)

set_target_properties(xdelta PROPERTIES PUBLIC_HEADER zenfs_xdelta3.h)
target_include_directories(xdelta PRIVATE include)

include(GNUInstallDirs)
install(TARGETS xdelta
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})