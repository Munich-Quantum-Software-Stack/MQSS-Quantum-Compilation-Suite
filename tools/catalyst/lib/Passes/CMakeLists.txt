set(PASS_ANALYSIS_SOURCES
  Analysis/PrintCatalystGates.cpp
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

if(BUILD_CATALYST)
  target_compile_definitions(MLIRPasses PRIVATE BUILD_CATALYST_ENABLED)
endif()

target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../InternalIR
)

