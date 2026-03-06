set(PASS_ANALYSIS_SOURCES
  Analysis/PrintQuakeGates.cpp
  Transforms/CommuteCxRx.cpp
  Transforms/CancellationDoubleCx.cpp
  Utils/QuakeUtils.cpp
  ${CMAKE_SOURCE_DIR}/../common/SemanticExtractLayer/Extractor.cpp
  ${CMAKE_SOURCE_DIR}/../common/Passes/GateCancellationPass.cpp
)


add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

  DEPENDS
    MQSSTransformsPassIncGen

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
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/../common
)

