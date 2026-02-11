cmake_minimum_required(VERSION 3.10)
project(Tests)
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

include_directories(${MLIR_INCLUDE_DIRS})

# Define the common libraries to link
set(COMMON_LIBRARIES ${CUDAQ_MLIR_RUNTIME_PATH}/libcudaq-mlir-runtime.so
                     pthread)

# quake-to-tikz executable
add_executable(quake-to-tikz quake-to-tikz.cpp)
target_link_libraries(quake-to-tikz PRIVATE ${COMMON_LIBRARIES} MLIRPasses
                                            Boost::program_options)
target_include_directories(quake-to-tikz PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                 ${CUDAQ_MLIR_RUNTIME_PATH})
# set_target_properties(quake-to-tikz PROPERTIES INSTALL_RPATH "$ORIGIN/../lib"
# BUILD_WITH_INSTALL_RPATH ON)

add_executable(qasm-to-quake qasm-to-quake.cpp)
target_link_libraries(qasm-to-quake PRIVATE ${COMMON_LIBRARIES} MLIRPasses
                                            Boost::program_options)
target_include_directories(qasm-to-quake PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                 ${CUDAQ_MLIR_RUNTIME_PATH})
# set_target_properties(qasm-to-quake PROPERTIES INSTALL_RPATH "$ORIGIN/../lib"
# BUILD_WITH_INSTALL_RPATH ON)

add_executable(cpp-to-only-quake cpp-to-only-quake.cpp)
target_link_libraries(cpp-to-only-quake PRIVATE ${COMMON_LIBRARIES} MLIRPasses
                                                Boost::program_options)
target_include_directories(cpp-to-only-quake PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                     ${CUDAQ_MLIR_RUNTIME_PATH})
# set_target_properties(cpp-to-only-quake PROPERTIES INSTALL_RPATH
# "$ORIGIN/../lib" BUILD_WITH_INSTALL_RPATH ON)

# 1. Define the executable and its source file
add_executable(mqss-opt mqss.cpp)

# 2. Link the required libraries
# Note: We must include the Dialects to resolve those "undefined symbols"
target_link_libraries(mqss-opt 
  PRIVATE 
    Optimizer               # Your MQSS passes
    MLIRPasses              # The library containing CommuteCxRx
    # The missing MLIR dialects
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    
    SupportQuake            # MQSS/Quake support
    Interfaces              # MQSS Interfaces
    MLIROptLib              # Provides MlirOptMain engine
    ${COMMON_LIBRARIES}     # Your CUDA-Q runtime + GTest
)

# 3. Set include directories so the compiler finds your headers
target_include_directories(mqss-opt
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Passes
    ${CUDAQ_MLIR_RUNTIME_PATH}/include  # Path to CUDA-Q headers
)

# 4. Ensure it outputs to build/bin
set_target_properties(mqss-opt PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# (Optional) Verify links
mlir_check_all_link_libraries(mqss-opt)

# Install the quake-to-tikz tool
install(TARGETS quake-to-tikz RUNTIME DESTINATION bin)

# Install the qasm-to-quake tool
install(TARGETS qasm-to-quake RUNTIME DESTINATION bin)
