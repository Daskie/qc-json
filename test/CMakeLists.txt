### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

### QC-JSON-TEST ###############################################################

qc_setup_target(
    qc-json-test
    EXECUTABLE
    SOURCE_FILES
        test-json.cpp
    PRIVATE_LINKS
        qc-json
        GTest::gtest_main
)

qc_setup_target(
    qc-json-decode-test
    EXECUTABLE
    SOURCE_FILES
        test-decode.cpp
    PRIVATE_LINKS
        qc-json
        GTest::gtest_main
)

qc_setup_target(
    qc-json-encode-test
    EXECUTABLE
    SOURCE_FILES
        test-encode.cpp
    PRIVATE_LINKS
        qc-json
        GTest::gtest_main
)
