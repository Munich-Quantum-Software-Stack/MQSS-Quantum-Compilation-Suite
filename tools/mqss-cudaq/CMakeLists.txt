
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

#########################LLVM-LIT TESTING SETTINGS###############################

set(LLVM_EXTERNAL_LIT "/workspaces/compilers/MQSS-Catalyst-Compiler/mlir/llvm-project/build/bin/llvm-lit")

message(CMAKESRCDIR= "${CMAKE_SOURCE_DIR}")
add_custom_target(check-all
  COMMAND ${LLVM_EXTERNAL_LIT} ${CMAKE_SOURCE_DIR}/../../test-mqss-passes/dialects/quake
)

################################-MQSS Settings-###############################

add_subdirectory(lib)
add_subdirectory(include)

#1. Define the executable and its source file
add_executable(mqss-cudaq-opt cudaq.cpp)

# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"

# 3. Set include directories so the compiler finds your headers
target_include_directories(mqss-cudaq-opt
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    #${CUDAQ_MLIR_RUNTIME_PATH}/include  # Path to CUDA-Q headers
)

if(BUILD_CUDAQ)
  target_compile_definitions(mqss-cudaq-opt PRIVATE BUILD_CUDAQ_ENABLED)
endif()

target_link_libraries(mqss-cudaq-opt 
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
set_target_properties(mqss-cudaq-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-cudaq-opt)