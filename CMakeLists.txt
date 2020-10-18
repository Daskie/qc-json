cmake_minimum_required(VERSION 3.17)
project(qc-json)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(extern)
add_subdirectory(test)

add_library(qc-json INTERFACE)

target_include_directories(qc-json INTERFACE include)
