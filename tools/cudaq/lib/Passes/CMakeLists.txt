set(PASS_ANALYSIS_SOURCES
  Analysis/PrintQuakeGates.cpp
)

add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

  LINK_LIBS PUBLIC
    MLIRTensorDialect
    MLIRIR
    MLIRPass
    MLIRTransforms
)

if(BUILD_CUDAQ)
  target_compile_definitions(MLIRPasses PRIVATE BUILD_CUDAQ_ENABLED)
endif()

target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../InternalIR
)

