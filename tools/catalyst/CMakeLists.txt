

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

find_package(MLIR REQUIRED CONFIG)
# if(NOT CATALYST_DOCS_ONLY)
#   find_package(MHLO REQUIRED CONFIG)
# endif()

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# list(APPEND CMAKE_MODULE_PATH "${MHLO_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CURRENT_DIR}/tools/catalyst/cmake/modules")

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

if(APPLE)
  string(REPLACE "-Wl,-undefined -Wl,suppress" "" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
endif()

if(QUANTUM_ENABLE_BINDINGS_PYTHON)
  include(MLIRDetectPythonEnv)
  mlir_configure_python_dev_packages()
endif()

# list(GET MHLO_INCLUDE_DIRS 1 MLIRHLO_DIR)
# list(GET MHLO_INCLUDE_DIRS 2 MLIRHLO_BUILD_DIR)

set(CATALYST_MAIN_INCLUDE_DIR ${DIALECTS_BUILD_DIR}/tools/catalyst/include/)
# set(CATALYST_GEN_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)
# set(CATALYST_LIB_DIR ${PROJECT_BINARY_DIR})

include_directories(BEFORE ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
#   ${MHLO_INCLUDE_DIRS}
#   ${MLIRHLO_DIR}/stablehlo
#   ${MLIRHLO_BUILD_DIR}/stablehlo
)
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

set(CATALYST_MLIR_RUNTIME_PATH "${DIALECTS_BUILD_DIR}/tools/catalyst/")

set(CUDAQ_BINARY_DIR "${CUDAQ_SOURCE_DIR}/build")

include_directories(BEFORE ${CATALYST_MAIN_INCLUDE_DIR}
  ${DIALECTS_BUILD_DIR}/tools/catalyst/include/IR
)


add_subdirectory(include) # Must come first to set up targets
add_subdirectory(lib)

# add_subdirectory(include)
# add_subdirectory(lib)
add_subdirectory(cmake/modules)

include(FetchContent)


add_executable(catalyst-opt catalyst.cpp)

# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"
target_link_libraries(catalyst-opt 
  PRIVATE 
    MLIRQuantum
    # The missing MLIR dialects
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRTransforms
    
    # SupportQuake            # MQSS/Quake support
    # Interfaces              # MQSS Interfaces
    MLIROptLib              # Provides MlirOptMain engine
)

# 3. Set include directories so the compiler finds your headers
target_include_directories(catalyst-opt
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    # ${CMAKE_SOURCE_DIR}/include/Passes
    # 2. Build Includes (CRITICAL): Allows finding the generated .inc files
    ${CMAKE_CURRENT_BINARY_DIR}/include
    
    ${CATALYST_MLIR_RUNTIME_PATH}/include/IR  # Path to CUDA-Q headers
)

# 4. Ensure it outputs to build/bin
set_target_properties(catalyst-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# (Optional) Verify links
mlir_check_all_link_libraries(catalyst-opt)