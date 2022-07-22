cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(custom_ops)

find_package(Torch REQUIRED)

add_executable(test_sim test_sim.cc hash_network.cc hash_pool.cc)
target_link_libraries(test_sim "${TORCH_LIBRARIES}")
target_link_libraries(test_sim -lngt)
set_property(TARGET test_sim PROPERTY CXX_STANDARD 14)