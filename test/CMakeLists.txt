### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

### QC-JSON-TEST ###############################################################

qc_setup_target(
    qc-json-test
    EXECUTABLE
    SOURCE_FILES
        *.cpp
    PRIVATE_LINKS
        qc-json
        GTest::gtest_main
)
