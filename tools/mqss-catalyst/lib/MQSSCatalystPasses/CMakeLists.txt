

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/../common/Passes/*.cpp")

set(PASS_ANALYSIS_SOURCES
  Analysis/PrintCatalystGates.cpp
  ${SOURCES}
  ${MQSS_COMMON}/Passes/Pipelines/pipeline.cpp
)

add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

   DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    MLIRQuantum
    MLIRTensorDialect
    MLIRArithDialect
    MLIRFuncDialect
    MLIRIR
    MLIRPass
    MLIRTransforms
    MQT::CoreIR
    MQT::CoreNA
    MQT::QMapSC
    MQT::QMapSCHeuristic
)

if(BUILD_CATALYST)
  target_compile_definitions(MLIRPasses PRIVATE BUILD_CATALYST_ENABLED)
  target_compile_options(MLIRPasses PRIVATE -fexceptions)
endif()

target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${MQSS_COMMON}
    ${nlohmann_json_SOURCE_DIR}/include
    ${MQT_QMAP_INCLUDE_DIR}
)

