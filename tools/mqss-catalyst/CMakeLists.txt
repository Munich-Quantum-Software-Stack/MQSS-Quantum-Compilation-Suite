

cmake_minimum_required(VERSION 3.20)
project(Catalyst LANGUAGES CXX C)

set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")

# Required so as not to always use the cached option from the mlir build.
option(QUANTUM_ENABLE_BINDINGS_PYTHON "Enable quantum dialect python bindings" OFF)
option(CATALYST_ENABLE_WARNINGS "Enable -Wall and -Werror" ON)
option(CATALYST_DOCS_ONLY "Disable targets that require built dependencies" OFF)

message(STATUS "LLVM_USE_SANITIZER is ${LLVM_USE_SANITIZER}.")
message(STATUS "CATALYST_ENABLE_WARNINGS is ${CATALYST_ENABLE_WARNINGS}.")

if ("${LLVM_USE_SANITIZER}" STREQUAL "Address")
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(CATALYST_ENABLE_WARNINGS)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
    add_compile_options(-Wall)
endif()

#########################
## Handle Dependencies ##
#########################

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

find_package(MLIR REQUIRED CONFIG)
# if(NOT CATALYST_DOCS_ONLY)
#   find_package(MHLO REQUIRED CONFIG)
# endif()

include_directories(${MLIR_INCLUDE_DIRS})

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# list(APPEND CMAKE_MODULE_PATH "${MHLO_CMAKE_DIR}")
# list(APPEND CMAKE_MODULE_PATH "${CURRENT_DIR}/tools/mqss-catalyst/cmake/modules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
# Policy CMP0175 was introduced in CMake 4.31 and raises warnings in the upstream CMake modules.
# Policy CMP0177 was introduced in CMake 4.31 and raises warnings in the upstream CMake modules.
# TODO: Remove once they (and us) have updated their code to deal with it.
if(POLICY CMP0175)
  cmake_policy(SET CMP0175 OLD)
endif()
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 OLD)
endif()

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

# if(APPLE)
#   string(REPLACE "-Wl,-undefined -Wl,suppress" "" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
# endif()

if(QUANTUM_ENABLE_BINDINGS_PYTHON)
  include(MLIRDetectPythonEnv)
  mlir_configure_python_dev_packages()
endif()

# list(GET MHLO_INCLUDE_DIRS 1 MLIRHLO_DIR)
# list(GET MHLO_INCLUDE_DIRS 2 MLIRHLO_BUILD_DIR)


# set(CATALYST_GEN_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)
# set(CATALYST_LIB_DIR ${PROJECT_BINARY_DIR})

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
#   ${MHLO_INCLUDE_DIRS}
#   ${MLIRHLO_DIR}/stablehlo
#   ${MLIRHLO_BUILD_DIR}/stablehlo
)
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

# ############### ADDITIONAL PACKAGES #######################################

# Tell where to search for Find<package>.cmake files
set(FETCHCONTENT_BASE_DIR ${MQSS_SRC_DIR}/_deps/mqss-catalyst CACHE PATH "FetchContent cache dir")

set(CMAKE_MODULE_PATH "${MQSS_SRC_DIR}/cmake" ${CMAKE_MODULE_PATH})

# set(CMAKE_CXX_STANDARD 11) Find QDMI package find_package(qdmi REQUIRED) Find
# QInfo package find_package(qinfo REQUIRED)
find_package(Z3 REQUIRED)

# find_package(mqt-qcec REQUIRED)
# Find the QMAP package
# set(CMAKE_CXX_STANDARD 17)
set(MQT_QMAP_TESTS OFF)
find_package(mqt-qmap REQUIRED)
# Restore the original CXX flags and compiler
set(CMAKE_CXX_STANDARD 20)

set(MQT_QMAP_INCLUDE_DIR "${mqt-qmap_SOURCE_DIR}/include/")
set(MQT_QCEC_INCLUDE_DIR "${mqt-qcec_SOURCE_DIR}/include/")
set(MQT_CORE_INCLUDE_DIR "${mqt-core_SOURCE_DIR}/include/mqt-core")
# set(qdmi_INCLUDE_DIRS "${QDMI_INCLUDE_DIRS}/qdmi")
# set(qinfo_INCLUDE_DIRS "${qinfo_SOURCE_DIR}/include/qinfo")

message("MQT_CORE_INCLUDE_DIR = ${MQT_CORE_INCLUDE_DIR}")
message("MQT_QMAP_INCLUDE_DIR = ${MQT_QMAP_INCLUDE_DIR}")
include_directories(
        BEFORE
        ${nlohmann_json_SOURCE_DIR}/include
        ${MQT_QMAP_INCLUDE_DIR}
        ${MQT_CORE_INCLUDE_DIR}
        # ${qdmi_INCLUDE_DIRS}
        # ${qinfo_INCLUDE_DIRS}
        )
  
########### MQSS Settings ########################

set(CATALYST_MLIR_RUNTIME_PATH "${MQSS_BUILD_DIR}/tools/mqss-catalyst/")

set(MQSS_BUILD_DIR "${MQSS_SRC_DIR}/build")
set(MQSS_CATALYST_MAIN_INCLUDE_DIR "${MQSS_SRC_DIR}/tools/mqss-catalyst/include")
set(MQSS_COMMON "${MQSS_SRC_DIR}/tools/common")

include_directories(BEFORE ${MQSS_CATALYST_MAIN_INCLUDE_DIR}
  ${MQSS_BUILD_DIR}/tools/mqss-catalyst/include
)


include(FetchContent)



#########################LLVM-LIT TESTING SETTINGS###############################
# TODO: Change this path. Should point to llvm-lit in an LLVM toolchain within build/_Deps
find_program(LLVM_EXTERNAL_LIT
  NAMES llvm-lit lit
  REQUIRED
)
add_custom_target(check-mqss
  COMMAND ${LLVM_EXTERNAL_LIT} ${MQSS_SRC_DIR}/tests/dialects/catalyst-quantum
  USES_TERMINAL
)

add_custom_target(check-mqss-code
  COMMAND ${LLVM_EXTERNAL_LIT} ${MQSS_SRC_DIR}/tests/code/catalyst/tests
  USES_TERMINAL
)


################################-MQSS Settings-###############################

add_subdirectory(include) # Must come first to set up targets
add_subdirectory(lib)

add_executable(mqss-catalyst-opt catalyst.cpp)

# 3. Set include directories so the compiler finds your headers
target_include_directories(mqss-catalyst-opt
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # ${CMAKE_SOURCE_DIR}/include/Passes
    # 2. Build Includes (CRITICAL): Allows finding the generated .inc files
    ${CMAKE_CURRENT_BINARY_DIR}/include
    #${MLIR_INCLUDE_DIRS}
)


# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"
target_link_libraries(mqss-catalyst-opt 
  PRIVATE 
    MLIRPasses
    MLIRQuantum
    # The missing MLIR dialects
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRLinalgDialect
    MLIRSCFDialect
    MLIRLLVMDialect
    MLIRTransforms
    MQT::CoreIR
    MQT::CoreNA
    MQT::QMapSC
    MQT::QMapSCHeuristic
    
    # SupportQuake            # MQSS/Quake support
    # Interfaces              # MQSS Interfaces
    MLIROptLib              # Provides MlirOptMain engine
)

# 4. Ensure it outputs to build/bin
set_target_properties(mqss-catalyst-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}"
)

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-catalyst-opt)