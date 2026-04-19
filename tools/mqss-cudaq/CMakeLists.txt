
# Project name and language specification
cmake_minimum_required(VERSION 3.19...3.30)

project(Tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")

# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_USE_MULTITHREADED ON)
# set(Boost_USE_STATIC_RUNTIME OFF)
# find_package(Boost REQUIRED COMPONENTS program_options)
# message(STATUS "Boost version: ${Boost_VERSION}")
# if(NOT Boost_FOUND)
#   message(FATAL_ERROR "Boost library not found.")
# endif()


set(MLIR_DIR "${MLIR_DIR}")
set(LLVM_DIR "${LLVM_DIR}")

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


# ################ LLVM/MLIR Settings ###########################################
# Find MLIR. (MLIR is usually installed as part of an LLVM distribution.)
find_package(MLIR REQUIRED CONFIG)
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

link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

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

# ###################### MQSS settings ###################################

set(MQSS_BUILD_DIR "${MQSS_SRC_DIR}/build")
set(MQSS_CUDAQ_MAIN_INCLUDE_DIR "${MQSS_SRC_DIR}/tools/mqss-cudaq/include")

include_directories(BEFORE ${MQSS_CUDAQ_MAIN_INCLUDE_DIR}
  ${MQSS_BUILD_DIR}/tools/mqss-cudaq/include
)

include(FetchContent)

#########################LLVM-LIT TESTING SETTINGS###############################

set(LLVM_EXTERNAL_LIT "/workspaces/compilers/MQSS-Catalyst-Compiler/mlir/llvm-project/build/bin/llvm-lit")

add_custom_target(check-mqss
  COMMAND ${LLVM_EXTERNAL_LIT} ${CMAKE_SOURCE_DIR}/../../test-mqss-passes/dialects/quake
)

################################-MQSS Settings-###############################

add_subdirectory(lib)
add_subdirectory(include)

#1. Define the executable and its source file
add_executable(mqss-cudaq-opt cudaq.cpp)
target_compile_definitions(mqss-cudaq-opt PRIVATE BUILD_CUDAQ_ENABLED)

# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"

# 3. Set include directories so the compiler finds your headers
target_include_directories(mqss-cudaq-opt
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CUDAQ_SOURCE_DIR}/include  # Path to CUDA-Q headers
)

# if(BUILD_CUDAQ)
#   target_compile_definitions(mqss-cudaq-opt PRIVATE BUILD_CUDAQ_ENABLED)
# endif()

target_link_libraries(mqss-cudaq-opt 
  PRIVATE 
    MLIRPasses
    CCDialect
    QuakeDialect
    # The missing MLIR dialects
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRLLVMDialect
    MLIRTransforms
    
    # SupportQuake            # MQSS/Quake support
    # Interfaces              # MQSS Interfaces
    MLIROptLib              # Provides MlirOptMain engine
    ${COMMON_LIBRARIES}     # Your CUDA-Q runtime + GTest
)

# 4. Ensure it outputs to build/bin
set_target_properties(mqss-cudaq-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/bin"
)

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-cudaq-opt)