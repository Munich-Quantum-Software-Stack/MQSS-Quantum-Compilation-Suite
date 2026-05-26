

file(GLOB COMMON_PASSES "${MQSS_COMMON}/Passes/*.cpp")
file(GLOB CATALYST_PASSES "Transforms/*.cpp")

set(PASS_ANALYSIS_SOURCES
  Analysis/PrintCatalystGates.cpp
  ${COMMON_PASSES}
  ${CATALYST_PASSES}
)

add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

   DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    MLIRQuantum
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRLinalgDialect
    MLIRSCFDialect
    MLIRLLVMDialect
    MLIRIR
    MLIRPass
    MLIRTransforms
    MQT::CoreIR
    MQT::CoreNA
    MQT::QMapSC
    MQT::QMapSCHeuristic
)

if(BUILD_CATALYST)
  target_compile_definitions(MLIRPasses PRIVATE BUILD_CATALYST_ENABLED MQSS_ENABLE_DEBUG)
  target_compile_options(MLIRPasses PRIVATE -fexceptions)
  if(MQSS_ENABLE_DEBUG)
    target_compile_definitions(MLIRPasses PRIVATE MQSS_ENABLE_DEBUG)
  endif()
endif()

target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${MQSS_COMMON}
    ${nlohmann_json_SOURCE_DIR}/include
    ${MQT_QMAP_INCLUDE_DIR}
)

