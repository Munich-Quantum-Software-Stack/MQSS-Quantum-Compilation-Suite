set(PASS_ANALYSIS_SOURCES
  Analysis/PrintCatalystGates.cpp
   ${CMAKE_SOURCE_DIR}/../common/Passes/GateCancellationPass.cpp
   ${CMAKE_SOURCE_DIR}/../common/Passes/GateCommutationPass.cpp
)

add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

   DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    MLIRQuantum
    MLIRTensorDialect
    MLIRFuncDialect
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
    ${CMAKE_SOURCE_DIR}/../common
)

