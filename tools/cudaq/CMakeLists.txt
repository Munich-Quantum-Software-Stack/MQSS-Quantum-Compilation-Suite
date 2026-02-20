
# Project name and language specification
cmake_minimum_required(VERSION 3.19...3.30)

project(Tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS program_options)
message(STATUS "Boost version: ${Boost_VERSION}")
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost library not found.")
endif()


set(MLIR_DIR "/usr/local/llvm/lib/cmake/mlir")
set(LLVM_DIR "/usr/local/llvm/lib/cmake/llvm")
set(CLANG_DIR "/usr/local/llvm/lib/cmake/clang")

include_directories(${MLIR_INCLUDE_DIRS})

# Set default build type to Debug if none is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type (Debug, Release)" FORCE)
endif()

# Define macros for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("Configuring for Debug mode")
  add_compile_definitions(DEBUG)
endif()


# ################ MLIR Settings ###########################################
# Find MLIR. (MLIR is usually installed as part of an LLVM distribution.)
find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

# Append MLIR's CMake modules.
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Include the MLIR helper macros.
include(TableGen)
include(AddMLIR)
include(AddLLVM)
# Add LLVM, Clang and MLIR headers to the include path
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})

# ############### ADDITIONAL PACKAGES #######################################
# Tell where to search for Find<package>.cmake files
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# set(CMAKE_CXX_STANDARD 11) Find QDMI package find_package(qdmi REQUIRED) Find
# QInfo package find_package(qinfo REQUIRED)
# find_package(Z3 REQUIRED)

# find_package(mqt-qcec REQUIRED)
# # Find the QMAP package
# set(CMAKE_CXX_STANDARD 17)
# set(MQT_QMAP_TESTS OFF)
# find_package(mqt-qmap REQUIRED)
# Restore the original CXX flags and compiler
set(CMAKE_CXX_STANDARD 20)

# ###################### CUDAQ settings ###################################

message("CUDAQ_DIR = ${CUDAQ_SOURCE_DIR}")

set(CUDAQ_MAIN_INCLUDE_DIR "${CUDAQ_SOURCE_DIR}/include")

set(CUDAQ_BINARY_DIR "${CUDAQ_SOURCE_DIR}/build")
set(CUDAQ_CMAKE_DIR "${CUDAQ_SOURCE_DIR}/cmake")
set(CUDAQ_INCLUDE_DIR "${CUDAQ_SOURCE_DIR}/build/include")
set(CUDAQ_LIBRARY_DIR "${CUDAQ_SOURCE_DIR}/build/lib")
set(CUDAQ_TOOLS_DIR "${CUDAQ_SOURCE_DIR}/build/bin")
set(CUDAQ_MLIR_RUNTIME_PATH "${CUDAQ_SOURCE_DIR}/build/lib")

include_directories(BEFORE ${CUDAQ_MAIN_INCLUDE_DIR}
                    ${CUDAQ_BINARY_DIR}/include ${CUDAQ_SOURCE_DIR}/runtime)

# Validate CUDAQ dependencies
if(NOT EXISTS ${CUDAQ_BINARY_DIR}/lib/libcudaq-mlir-runtime.so)
  message(
    FATAL_ERROR
      "Invalid CUDAQ dir: ${CUDAQ_SOURCE_DIR}. Cannot find libcudaq.so in build directory."
  )
endif()

include(FetchContent)

# Define the common libraries to link
set(COMMON_LIBRARIES ${CUDAQ_MLIR_RUNTIME_PATH}/libcudaq-mlir-runtime.so
                     pthread)

# ##############################################################################

##################################-MQSS Settings-###############################

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)
# include_directories(
#   BEFORE
#   ${CMAKE_SOURCE_DIR}/include
#   ${CMAKE_SOURCE_DIR}/include/Passes/
#   )

# message(CMAKE_BIN_DIR= "${CMAKE_BINARY_DIR}")

# file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/Passes)
# # file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/Optimizer)

# # set(LLVM_TARGET_DEFINITIONS
# #     ${CMAKE_CURRENT_SOURCE_DIR}/include/Passes/Transforms.td)
# # # Generate .inc files from TableGen
# # mlir_tablegen(include/Passes/Transforms.h.inc -gen-pass-decls -name
# #               MQSSOptTransforms)
# # add_public_tablegen_target(MQSSTransformsPassIncGen)

# # set(LLVM_TARGET_DEFINITIONS
# #     ${CMAKE_CURRENT_SOURCE_DIR}/include/Passes/Decompositions.td)
# # # Generate .inc files from TableGen
# # mlir_tablegen(include/Passes/Decompositions.h.inc -gen-pass-decls -name
# #               MQSSOptDecompositions)
# # add_public_tablegen_target(MQSSDecompositionsPassIncGen)

# include_directories(BEFORE ${CMAKE_BINARY_DIR}/include)

# file(GLOB SRC_ANALYSIS "${CMAKE_SOURCE_DIR}/lib/Passes/Analysis/*.cpp")

# # file(GLOB SRC_TRANSFORMS
# #      "${CMAKE_SOURCE_DIR}/lib/Passes/Transforms/*.cpp")

# message(STATUS "CURRENT SOURCE DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

# add_mlir_library(
#   MLIRPasses
#   SHARED
#   # ${SRC_CODEGEN}
#   # ${SRC_DECOMPOSITIONS}
#   ${SRC_ANALYSIS}
#   #${SRC_TRANSFORMS}
#   # DEPENDS
#   # # MQSSTransformsPassIncGen
#   # # MQSSDecompositionsPassIncGen
#   # LINK_LIBS
#   # PUBLIC
#   # SupportQuake
#   # Interfaces
#   # MQT::CoreIR
#   # MQT::CoreNA
#   # MQT::QMapSC
#   # MQT::QMapSCHeuristic
#   MLIRTensorDialect
#   MLIRIR
#   MLIRPass
#   MLIRTransforms
#   )


# message(STATUS "CMAKE_SRC = ${CMAKE_CURRENT_SOURCE_DIR}")

# target_compile_options(MLIRPasses PRIVATE -fexceptions -frtti)


# # Create an alias target for convenience
# add_library(MQSS::MLIRPasses ALIAS MLIRPasses)

# install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

#################################################################################
# Enable FetchContent for integration

#1. Define the executable and its source file
add_executable(cudaq-opt cudaq.cpp)

# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"
message(LIB CMAKE CUR SRC DIR= "${CMAKE_CURRENT_SOURCE_DIR}")
# 3. Set include directories so the compiler finds your headers
target_include_directories(cudaq-opt
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    #${CUDAQ_MLIR_RUNTIME_PATH}/include  # Path to CUDA-Q headers
)

target_link_libraries(cudaq-opt 
  PRIVATE 
    MLIRPasses
    # The missing MLIR dialects
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRTransforms
    
    # SupportQuake            # MQSS/Quake support
    # Interfaces              # MQSS Interfaces
    MLIROptLib              # Provides MlirOptMain engine
    ${COMMON_LIBRARIES}     # Your CUDA-Q runtime + GTest
)

# 4. Ensure it outputs to build/bin
set_target_properties(cudaq-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# (Optional) Verify links
mlir_check_all_link_libraries(cudaq-opt)