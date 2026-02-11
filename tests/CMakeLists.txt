cmake_minimum_required(VERSION 3.10)
project(ProjectTests)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")
# Add Google Test
find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})

# Define the common libraries to link
set(COMMON_LIBRARIES ${CUDAQ_MLIR_RUNTIME_PATH}/libcudaq-mlir-runtime.so
                     ${GTEST_LIBRARIES})

# Test executable
add_executable(test-linalg-pass testLinAlgParser.cpp)
target_link_libraries(test-linalg-pass PRIVATE ${COMMON_LIBRARIES} MLIRPasses)
target_include_directories(
  test-linalg-pass
  PRIVATE ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${CUDAQ_MLIR_RUNTIME_PATH})
add_test(NAME Tesst-lingalg-pass COMMAND test-linalg-pass)

add_executable(test-mqss-passes testMQSSPasses.cpp)
target_link_libraries(test-mqss-passes PRIVATE ${COMMON_LIBRARIES} MLIRPasses)
target_include_directories(
  test-mqss-passes
  PRIVATE ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${CUDAQ_MLIR_RUNTIME_PATH})
add_test(NAME Tesst-MQSS-passes COMMAND test-mqss-passes)

add_executable(test-mytest testMytest.cpp)
target_link_libraries(test-mytest PRIVATE ${COMMON_LIBRARIES} MLIRPasses)
target_include_directories(
  test-mytest
  PRIVATE ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${CUDAQ_MLIR_RUNTIME_PATH})
          

# add_executable(test-decomposition-passes testCudaQDecompositionPasses.cpp)
# target_link_libraries(test-decomposition-passes PRIVATE ${COMMON_LIBRARIES})
# target_include_directories(
#   test-decomposition-passes PRIVATE ${CMAKE_SOURCE_DIR}/include
#                                     ${CUDAQ_MLIR_RUNTIME_PATH})

add_executable(test-transpiler testCudaQTranspiler.cpp)
target_link_libraries(test-transpiler PRIVATE ${COMMON_LIBRARIES})
target_include_directories(test-transpiler PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                   ${CUDAQ_MLIR_RUNTIME_PATH})

add_executable(test-verification-qcec testMQSS-QCEC.cpp)
target_link_libraries(test-verification-qcec PRIVATE ${COMMON_LIBRARIES}
                                                     MQT::QCEC MLIRPasses)

target_include_directories(
  test-verification-qcec
  PRIVATE # ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${MQT_QCEC_INCLUDE_DIR}
          ${CUDAQ_MLIR_RUNTIME_PATH})
add_test(NAME Test-verification-QCEC COMMAND test-verification-qcec)

add_executable(test-qasm-parser testQASMParser.cpp)
set_target_properties(test-qasm-parser PROPERTIES GOOGLE_TEST_FLAGS
                                                  "--gtest_disable_parallel")
target_compile_options(test-qasm-parser PRIVATE -fsanitize=address -g -O1
                                                -fno-omit-frame-pointer)
target_link_libraries(
  test-qasm-parser PRIVATE ${COMMON_LIBRARIES} ${LIBZIP_LIBRARIES} MQT::QCEC
                           MLIRPasses -fsanitize=address) # sanitize is useful
                                                          # when struggling with
                                                          # memory problems
target_include_directories(
  test-qasm-parser
  PRIVATE ${LIBZIP_INCLUDE_DIRS}
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${MQT_QCEC_INCLUDE_DIR}
          ${CUDAQ_MLIR_RUNTIME_PATH})
# add_test(NAME Test-QASM-parser COMMAND test-qasm-parser)

add_executable(test-qasm-parser-simple testQASMParserSimple.cpp)
set_target_properties(test-qasm-parser-simple
                      PROPERTIES GOOGLE_TEST_FLAGS "--gtest_disable_parallel")
target_compile_options(
  test-qasm-parser-simple PRIVATE -fsanitize=address -g -O1
                                  -fno-omit-frame-pointer)
target_link_libraries(
  test-qasm-parser-simple PRIVATE ${COMMON_LIBRARIES} ${LIBZIP_LIBRARIES}
                                  MQT::QCEC MLIRPasses -fsanitize=address
)# sanitize is useful when struggling with memory problems
target_include_directories(
  test-qasm-parser-simple
  PRIVATE ${LIBZIP_INCLUDE_DIRS}
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${MQT_QCEC_INCLUDE_DIR}
          ${CUDAQ_MLIR_RUNTIME_PATH})
add_test(NAME Test-QASM-parser-simple COMMAND test-qasm-parser-simple)

add_executable(test-qasm-parser-correctness testQASMParserCorrectness.cpp)
set_target_properties(test-qasm-parser-correctness
                      PROPERTIES GOOGLE_TEST_FLAGS "--gtest_disable_parallel")
target_compile_options(
  test-qasm-parser-correctness PRIVATE -fsanitize=address -g -O1
                                       -fno-omit-frame-pointer)
target_link_libraries(
  test-qasm-parser-correctness PRIVATE ${COMMON_LIBRARIES} ${LIBZIP_LIBRARIES}
                                       MQT::QCEC MLIRPasses -fsanitize=address
)# sanitize is useful when struggling with memory problems
target_include_directories(
  test-qasm-parser-correctness
  PRIVATE ${LIBZIP_INCLUDE_DIRS}
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/Passes/
          ${CMAKE_SOURCE_DIR}/include/Support/
          ${CMAKE_SOURCE_DIR}/include/Support/CodeGen
          ${MQT_QCEC_INCLUDE_DIR}
          ${CUDAQ_MLIR_RUNTIME_PATH})
add_test(NAME Test-QASM-parser-correctness COMMAND test-qasm-parser-correctness)

set(TEST_OUTPUT_DIR $<TARGET_FILE_DIR:test-mqss-passes>/golden-cases)
add_custom_target(
  copy-golden-cases ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/tests/golden-cases ${TEST_OUTPUT_DIR})

set(TEST_OUTPUT_DIR $<TARGET_FILE_DIR:test-mqss-passes>/code)
add_custom_target(
  copy-code-test ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
                             ${CMAKE_SOURCE_DIR}/tests/code ${TEST_OUTPUT_DIR})

set(TEST_OUTPUT_DIR $<TARGET_FILE_DIR:test-mqss-passes>/quake)
add_custom_target(
  copy-quake-test ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/quake
          ${TEST_OUTPUT_DIR})

set(TEST_OUTPUT_DIR $<TARGET_FILE_DIR:test-mqss-passes>/qasm)
add_custom_target(
  copy-qasm-test ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
                             ${CMAKE_SOURCE_DIR}/tests/qasm ${TEST_OUTPUT_DIR})
