# add CXX language support
enable_language(CXX)

# ------------------------------------------------------------------------------
# Interfaces Tests
# ------------------------------------------------------------------------------
set(INPUTS_DIR "${CMAKE_SOURCE_DIR}/tests/unittests/input")

# create an executable in which the tests will be stored
add_executable(MQSSCIInterface_test test_Interface.cpp)

target_compile_definitions(MQSSCIInterface_test
                           PRIVATE MQSSCI_TEST_FIXTURE_DIR="${INPUTS_DIR}")

# link the Google test infrastructure to the test executable.
target_link_libraries(MQSSCIInterface_test
                      PRIVATE mqss-ci::mqss-ci GTest::gtest_main GTest::gmock)

target_compile_features(MQSSCIInterface_test PRIVATE cxx_std_20)

# discover tests
gtest_discover_tests(
  MQSSCIInterface_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
             DISCOVERY_TIMEOUT 60)
