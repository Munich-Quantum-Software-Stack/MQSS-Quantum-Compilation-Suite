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

# ################ LLVM/MLIR Settings
# ##############################################################################
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

# ############### ADDITIONAL PACKAGES #######################################

# Tell where to search for Find<package>.cmake files

# set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps CACHE PATH
# "FetchContent cache dir")

set(CMAKE_MODULE_PATH "${MQSSCI_ROOT_DIR}/cmake" ${CMAKE_MODULE_PATH})

# set(CMAKE_CXX_STANDARD 11) Find QDMI package find_package(qdmi REQUIRED) Find
# QInfo package find_package(qinfo REQUIRED) find_package(Z3 REQUIRED)

# find_package(mqt-qcec REQUIRED) Find the QMAP package set(CMAKE_CXX_STANDARD
# 17)
set(MQT_QMAP_TESTS OFF)
find_package(CUDAQ REQUIRED)
find_package(Catalyst REQUIRED)
find_package(qdmi REQUIRED)
find_package(mqt-qmap REQUIRED)
# Restore the original CXX flags and compiler
set(CMAKE_CXX_STANDARD 20)

set(MQT_QMAP_INCLUDE_DIR "${mqt-qmap_SOURCE_DIR}/include/")
set(MQT_QCEC_INCLUDE_DIR "${mqt-qcec_SOURCE_DIR}/include/")
set(MQT_CORE_INCLUDE_DIR "${mqt-core_SOURCE_DIR}/include/mqt-core")
set(QDMI_INCLUDE_DIR "${qdmi_SOURCE_DIR}/include")
set(QDMI_EXAMPLES_DIR "${qdmi_SOURCE_DIR}/examples/")

include_directories(
  BEFORE
  ${nlohmann_json_SOURCE_DIR}/include
  ${QDMI_INCLUDE_DIR}
  ${QDMI_EXAMPLES_DIR}
  ${MQT_QMAP_INCLUDE_DIR}
  ${CUDAQ_INCLUDE_DIRS}
  ${CATALYST_INCLUDE_DIRS}
  ${MQSSCI_ROOT_DIR}/include
  ${MQSSCI_ROOT_DIR_BINARY}/include)

include(FetchContent)

# ########################LLVM-LIT TESTING
# SETTINGS###############################

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

# ###############################-MQSS Settings-###############################

add_custom_target(mqss-cudaq-headers)
set_target_properties(mqss-cudaq-headers PROPERTIES FOLDER "Misc")

# Add subdirectories
add_subdirectory(include)
add_subdirectory(lib)

# Adding the Passes as an external library
add_library(mqss-ci::mqss-ci ALIAS MQSSCIPasses)

# Define the executable and its source file
add_executable(mqss-opt mqss-cc.cpp)

# Set include directories so the compiler finds the headers
target_include_directories(mqss-opt PRIVATE ${MQSSCI_ROOT_DIR}/include
                                            ${MQSSCI_ROOT_DIR_BINARY}/include)

# Link the dialect implementations and other dependencies
target_link_libraries(mqss-opt PUBLIC MLIROptLib MQSSCIPasses)

# Ensure it outputs to INSTALL_DIR
set_target_properties(mqss-opt PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                          "${INSTALL_DIR}")

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-opt)
