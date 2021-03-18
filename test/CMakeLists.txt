include(FetchContent)

### DEPENDENCIES ###

# googletest
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git)
FetchContent_MakeAvailable(googletest)

### QC-JSON-TEST ###

file(GLOB qc-json-test_SRC *.cpp)

add_executable(qc-json-test ${qc-json-test_SRC})

target_link_libraries(
    qc-json-test
    PRIVATE
        qc-json
        gtest_main
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(qc-json-test PRIVATE -Wno-unused-parameter)
endif ()
