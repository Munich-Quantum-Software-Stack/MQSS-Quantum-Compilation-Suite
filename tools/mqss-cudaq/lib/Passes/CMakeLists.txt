


file(GLOB COMMON_PASSES "${MQSS_COMMON}/Passes/*.cpp")
file(GLOB CUDAQ_PASSES "Transforms/*.cpp")

set(PASS_ANALYSIS_SOURCES
  Analysis/PrintQuakeGates.cpp
  ${COMMON_PASSES}
  ${CUDAQ_PASSES}
  ${MQSS_COMMON}/Passes/pass_pipeline/pipeline.cpp
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

