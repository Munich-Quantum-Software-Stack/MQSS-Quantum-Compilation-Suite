add_mlir_library(
  MQSSCITransformPasses
  CommonCNOTReversalPass.cpp
  CommonGateCancellationPass.cpp
  CommonGateCommutationPass.cpp
  CommonCommuteAndSwitchPass.cpp
  CommonDecompositionPass.cpp
  CommonPatternReductionPass.cpp
  CommonMappingPass.cpp
  CommonNormalizeArgAnglePass.cpp
  Pipeline.cpp
  QuakeExpandMeasurements.cpp
  DEPENDS
  MQSSTransformsPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSCICodeGenPasses
  qdmi::qdmi
  qdmi::example_driver
  MQT::CoreIR
  MQT::QMapSCHeuristic
  MLIRPass
  MLIRTransforms)

set_target_properties(
  MQSSCITransformPasses
  PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
             INSTALL_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>")

target_compile_definitions(MQSSCITransformPasses PRIVATE ${MLIR_DEFINITIONS}
                                                         ${LLVM_DEFINITIONS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(MQSSCITransformPasses PRIVATE MQSS_ENABLE_DEBUG)
endif()
target_compile_options(MQSSCITransformPasses PRIVATE -fexceptions)
if(TARGET obj.MQSSCITransformPasses)
  target_compile_options(obj.MQSSCITransformPasses PRIVATE -fexceptions)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(obj.MQSSCITransformPasses
                               PRIVATE MQSS_ENABLE_DEBUG)
  endif()
endif()

target_include_directories(
  MQSSCITransformPasses
  PUBLIC $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR}/include>
         $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR_BINARY}/include>
         ${nlohmann_json_SOURCE_DIR}/include
         ${QDMI_INCLUDE_DIR}
         ${MQT_QMAP_INCLUDE_DIR}
         ${LLVM_INCLUDE_DIRS}
         ${MLIR_INCLUDE_DIRS})
