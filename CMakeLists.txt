cmake_minimum_required(VERSION 3.23)

project(qc-json)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

qc_setup_target(qc-json INTERFACE_LIBRARY)

add_subdirectory(test)

qc_setup_install(TARGETS qc-json)
