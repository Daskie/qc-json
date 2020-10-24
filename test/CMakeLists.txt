# qc-json-test

add_executable(qc-json-test test-decode.cpp test-encode.cpp test-json.cpp)

target_link_libraries(qc-json-test PRIVATE gtest gtest_main qc-json)

target_include_directories(qc-json-test PRIVATE ${gtest_SOURCE_DIR}/include)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(qc-json-test PRIVATE -Wno-unused-parameter)
endif()
