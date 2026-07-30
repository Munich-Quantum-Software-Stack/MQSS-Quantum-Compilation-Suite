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

# Append MLIR's CMake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
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
set(FETCHCONTENT_BASE_DIR
    ${BUILD_DIR}/_deps
    CACHE PATH "FetchContent cache dir")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

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
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include)

include(FetchContent)

# ########################LLVM-LIT TESTING
# SETTINGS###############################

# TODO: Change this path. Should point to llvm-lit in an LLVM toolchain within
# build/_Deps

find_program(LLVM_EXTERNAL_LIT NAMES llvm-lit lit REQUIRED)

add_custom_target(
  check-mqss
  COMMAND ${LLVM_EXTERNAL_LIT} ${CMAKE_CURRENT_SOURCE_DIR}/tests/dialects/quake
          ${CMAKE_CURRENT_SOURCE_DIR}/tests/dialects/catalyst-quantum
  USES_TERMINAL)

add_custom_target(
  check-mqss-code
  COMMAND
    ${LLVM_EXTERNAL_LIT} ${CMAKE_CURRENT_SOURCE_DIR}/tests/code/cudaq/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/code/catalyst/tests
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
target_compile_definitions(
  mqss-opt PRIVATE ${MLIR_DEFINITIONS} ${LLVM_DEFINITIONS}
                   QDMI_DEVICE_PATH="$<TARGET_FILE:cxx-qdmi-device>")

target_compile_options(mqss-opt PRIVATE -fexceptions)

# Set include directories so the compiler finds the headers
target_include_directories(mqss-opt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                                            ${CMAKE_CURRENT_BINARY_DIR}/include)

# Link the dialect implementations and other dependencies
target_link_libraries(
  mqss-opt
  PUBLIC qdmi::qdmi
         qdmi::example_driver
         CUDAQ::CC
         CUDAQ::Quake
         CUDAQ::QEC
         CATALYST::MBQC
         CATALYST::QRef
         CATALYST::Quantum
         MLIRLLVMDialect
         MLIROptLib
         MLIRIR
         MLIRFuncDialect
         MLIRFuncInlinerExtension
         MLIRArithDialect
         MQSSCIPasses
         CUDAQ::CodeGen
         MLIRTransforms
         MQT::CoreIR
         MQT::CoreNA
         MQT::QMapSC
         MQT::QMapSCHeuristic
         MLIROptLib # Provides MlirOptMain engine
         # Conversions
         MLIRArithToLLVM
         MLIRComplexToLibm
         MLIRComplexToLLVM
         MLIRControlFlowToLLVM
         MLIRFuncToLLVM
         MLIRMathToFuncs
         MLIRMathToLLVM
         # Translation
         MLIRTargetLLVMIRExport)

# Ensure it outputs to INSTALL_DIR
set_target_properties(mqss-opt PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                          "${INSTALL_DIR}")

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-opt)
