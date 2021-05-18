### DEPENDENCIES ###

# googletest
find_package(GTest CONFIG REQUIRED)

### QC-JSON-TEST ###

file(GLOB qc-json-test_SRC *.cpp)

add_executable(qc-json-test ${qc-json-test_SRC})

# Link executable statically
set_property(TARGET qc-json-test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-json-test PRIVATE /W4 /WX)
else()
    add_compile_options(qc-json-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-json-test
    PRIVATE
        qc-json
        GTest::gtest_main
)
