### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

### QC-JSON-TEST ###############################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-json-test ${SRC_FILES})

if(MSVC)
    target_compile_options(qc-json-test PRIVATE /W4 /WX)
else()
    message(FATAL_ERROR "TODO")
endif()

target_link_libraries(
    qc-json-test
    PRIVATE
        qc-json
        GTest::gtest_main
)
