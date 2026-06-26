

file(GLOB COMMON_PASSES "${MQSS_COMMON}/Passes/*.cpp")
file(GLOB CATALYST_PASSES "Transforms/*.cpp")

set(PASS_ANALYSIS_SOURCES
  Analysis/PrintCatalystGates.cpp
  ${MQSS_COMMON}/QDMI/Driver.cpp
  ${COMMON_PASSES}
  ${CATALYST_PASSES}
)

add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

   DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    qdmi::qdmi
    qdmi::example_driver
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

set_target_properties(MLIRPasses PROPERTIES
  BUILD_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
  INSTALL_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
)

if(BUILD_CATALYST)
  target_compile_definitions(MLIRPasses PRIVATE BUILD_CATALYST_ENABLED
                                QDMI_DEVICE_PATH="$<TARGET_FILE:cxx-qdmi-device>")
  target_compile_options(MLIRPasses PRIVATE -fexceptions)
  if(MQSS_ENABLE_DEBUG)
    target_compile_definitions(MLIRPasses PRIVATE MQSS_ENABLE_DEBUG
                                    QDMI_DEVICE_PATH="$<TARGET_FILE:cxx-qdmi-device>")
  endif()
endif()

target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
    ${MQT_QMAP_INCLUDE_DIR}
    ${MQSS_COMMON}
)
