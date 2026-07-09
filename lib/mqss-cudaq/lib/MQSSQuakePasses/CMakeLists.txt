


file(GLOB COMMON_PASSES "${MQSS_COMMON}/Passes/*.cpp")
file(GLOB CUDAQ_PASSES "Transforms/*.cpp")

set(PASS_ANALYSIS_SOURCES
  Analysis/PrintQuakeGates.cpp
  ${COMMON_PASSES}
  ${CUDAQ_PASSES}
)


add_mlir_library(MLIRPasses
  SHARED
  ${PASS_ANALYSIS_SOURCES}

  DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    qdmi::qdmi
    qdmi::example_driver
    qdmi::qdmi_project_warnings
    CCDialect
    QuakeDialect
    MLIRTensorDialect
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

if(BUILD_CUDAQ)

  target_compile_definitions(MLIRPasses PRIVATE
    BUILD_CUDAQ_ENABLED
    ${MLIR_DEFINITIONS}
    ${LLVM_DEFINITIONS}
  )
  
  if(MQSS_ENABLE_DEBUG)
    target_compile_definitions(MLIRPasses PRIVATE
      MQSS_ENABLE_DEBUG
      ${MLIR_DEFINITIONS}
      ${LLVM_DEFINITIONS}
    )
  endif()
  
  target_compile_options(MLIRPasses PRIVATE -fexceptions)
endif()


target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
    ${QDMI_INCLUDE_DIR}
    ${QDMI_EXAMPLES_DIR}
    ${MQT_QMAP_INCLUDE_DIR}
    ${MQSS_COMMON}
)


