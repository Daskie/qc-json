### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

### QC-JSON-TEST ###############################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-json-test ${SRC_FILES})

target_compile_options(qc-json-test PRIVATE ${QC_WARNINGS_ERROR})

target_link_libraries(
    qc-json-test
    PRIVATE
        qc-json
        GTest::gtest_main
)
