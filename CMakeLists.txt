cmake_minimum_required(VERSION 3.17)
project(qc-json)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(extern)
add_subdirectory(test)

# qc-json

add_library(qc-json INTERFACE)

target_include_directories(qc-json INTERFACE include)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(qc-json INTERFACE -Wall -Werror -Wextra -Wpedantic -Wno-strict-aliasing)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(qc-json INTERFACE /W4 /WX)
endif()
