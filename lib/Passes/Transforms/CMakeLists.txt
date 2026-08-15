add_mlir_library(
  MQSSCIPasses
  SHARED
  CommonCNOTReversalPass.cpp
  CommonGateCancellationPass.cpp
  CommonGateCommutationPass.cpp
  CommonCommuteAndSwitchPass.cpp
  CommonDecompositionPass.cpp
  CommonPatternReductionPass.cpp
  CommonMappingPass.cpp
  CommonNormalizeArgAnglePass.cpp
  Pipeline.cpp
  DEPENDS
  MQSSTransformsPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSCICodeGenPasses
  qdmi::qdmi
  qdmi::example_driver
  MQT::CoreNA
  MQT::QMapSCHeuristic
  MLIRPass
  MLIRTransforms)

set_target_properties(
  MQSSCIPasses
  PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
             INSTALL_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>")

target_compile_definitions(MQSSCIPasses PRIVATE ${MLIR_DEFINITIONS}
                                                ${LLVM_DEFINITIONS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(MQSSCIPasses PRIVATE MQSS_ENABLE_DEBUG)
endif()
target_compile_options(MQSSCIPasses PRIVATE -fexceptions)

target_include_directories(
  MQSSCIPasses
  PUBLIC $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR}/include>
         $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR_BINARY}/include>
         ${nlohmann_json_SOURCE_DIR}/include
         ${QDMI_INCLUDE_DIR}
         ${MQT_QMAP_INCLUDE_DIR}
         ${LLVM_INCLUDE_DIRS}
         ${MLIR_INCLUDE_DIRS})
