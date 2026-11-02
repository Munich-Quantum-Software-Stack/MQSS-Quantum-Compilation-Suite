# Project name and language specification
cmake_minimum_required(VERSION 3.19...3.30)
project(MQSS-CC LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")

include_directories(${MLIR_INCLUDE_DIRS})

# Set default build type to Release if none is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type (Debug, Release)" FORCE)
endif()

# ################ LLVM/MLIR Settings ################################
# Find MLIR. (MLIR is usually installed as part of an LLVM distribution.)
set(MQSSCI_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSSCI_ROOT_DIR_BINARY "${CMAKE_CURRENT_BINARY_DIR}")

# Append MLIR's CMake modules.
list(APPEND CMAKE_MODULE_PATH "${MQSSCI_ROOT_DIR}/cmake")
include(FetchLLVM)
find_package(MLIR REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# Include the MLIR helper macros.
include(TableGen)
include(AddMLIR)
include(AddLLVM)
# Add LLVM, Clang and MLIR headers to the include path
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})

link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS} ${MLIR_DEFINITIONS})

# ############### ADDITIONAL LIBRARIES #############################

# Tell where to search for Find<package>.cmake files

set(CMAKE_MODULE_PATH "${MQSSCI_ROOT_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(MQT_QMAP_TESTS OFF)
find_package(CUDAQ REQUIRED)
find_package(Catalyst REQUIRED)
find_package(qdmi REQUIRED)
find_package(mqt-core REQUIRED)
find_package(mqt-qmap REQUIRED)
find_package(mqt-qcec REQUIRED)
find_package(GTest REQUIRED)

include_directories(
  BEFORE
  ${nlohmann_json_SOURCE_DIR}/include
  ${QDMI_INCLUDE_DIR}
  ${QDMI_EXAMPLES_DIR}
  ${MQT_QMAP_INCLUDE_DIR}
  ${MQT_QCEC_INCLUDE_DIR}
  ${MQT_CORE_INCLUDE_DIR}
  ${CUDAQ_INCLUDE_DIRS}
  ${CATALYST_INCLUDE_DIRS}
  ${MQSSCI_ROOT_DIR}/include
  ${MQSSCI_ROOT_DIR_BINARY}/include)

include(FetchContent)

# ####################### LLVM-LIT SETTINGS ###############################

find_program(LIT_EXECUTABLE lit)
if(LIT_EXECUTABLE)
  set(EXTERNAL_LIT
      "${LIT_EXECUTABLE}"
      CACHE FILEPATH "Path to llvm-lit" FORCE)
endif()

add_custom_target(
  check-mqss
  COMMAND ${EXTERNAL_LIT} ${MQSSCI_ROOT_DIR}/tests/dialects/quake
          ${MQSSCI_ROOT_DIR}/tests/dialects/catalyst-quantum
  USES_TERMINAL)

add_custom_target(
  check-mqss-code
  COMMAND ${EXTERNAL_LIT} ${MQSSCI_ROOT_DIR}/tests/code/cudaq/tests
          ${MQSSCI_ROOT_DIR}/tests/code/catalyst/tests
  USES_TERMINAL)

# ###################### MQSS Settings ###############################

# Add subdirectories
add_subdirectory(include)
add_subdirectory(lib)

# Exposing the MQSSCIInterfaces as an external library.
add_library(mqss-ci::mqss-ci ALIAS MQSSCIInterfaces)

# Define the executable and its source file
add_executable(mqss-opt mqss-cc.cpp)

# Set include directories so the compiler finds the headers
target_include_directories(mqss-opt PRIVATE ${MQSSCI_ROOT_DIR}/include
                                            ${MQSSCI_ROOT_DIR_BINARY}/include)

# Link the dialect implementations and other dependencies
target_link_libraries(mqss-opt PUBLIC MLIROptLib MQSSCITransformPasses
                                      MQSSCIVerificationPasses)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(mqss-opt PRIVATE MQSS_ENABLE_DEBUG)
endif()

# Ensure it outputs to INSTALL_DIR
set_target_properties(mqss-opt PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                          "${INSTALL_DIR}")

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-opt)

# add Interface Tests
if(BUILD_INTERFACES_TESTS)
  include(GoogleTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# QASM2 -> Quake frontend, built next to mqss-opt.
add_executable(mqss-translate mqss-translate.cpp)
target_include_directories(
  mqss-translate PRIVATE ${MQSSCI_ROOT_DIR}/include
                         ${MQSSCI_ROOT_DIR_BINARY}/include)
target_link_libraries(mqss-translate PUBLIC MQSSCITranslation MLIRTranslateLib)
set_target_properties(mqss-translate PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                "${INSTALL_DIR}")
