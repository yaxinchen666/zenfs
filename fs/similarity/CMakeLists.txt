cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(blksim)

add_library(blksim SHARED
    src/hash_network.cc
    src/hash_pool.cc
)

set_target_properties(blksim PROPERTIES PUBLIC_HEADER include/block_similarity.h)
target_include_directories(blksim PRIVATE include)

find_package(Torch REQUIRED)

target_link_libraries(blksim "${TORCH_LIBRARIES}")
target_link_libraries(blksim -lngt)

include(GNUInstallDirs)
install(TARGETS blksim
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})