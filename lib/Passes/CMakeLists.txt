add_mlir_library(
  MQSSCIPasses
  SHARED
  Transforms/CommonCNOTReversalPass.cpp
  Transforms/CommonGateCancellationPass.cpp
  Transforms/CommonGateCommutationPass.cpp
  Transforms/CommonCommuteAndSwitchPass.cpp
  Transforms/CommonDecompositionPass.cpp
  Transforms/CommonPatternReductionPass.cpp
  Transforms/CommonMappingPass.cpp
  Transforms/CommonNormalizeArgAnglePass.cpp
  Transforms/Pipeline.cpp
  CodeGen/LLVMDialectToLLVMIR.cpp
  CodeGen/QuakeToQASM2.cpp
  CodeGen/ConversionPatterns.cpp
  CodeGen/QuantumToLLVMDialect.cpp
  CodeGen/BasisConversion.cpp
  CodeGen/GlobalizeArrayValues.cpp
  CodeGen/StaticAllocas.cpp

  DEPENDS
  MQSSTransformsPassIncGen
  
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  qdmi::qdmi
  qdmi::example_driver
  MLIRPass
  MLIRTransforms
  CUDAQ::CodeGen
  CUDAQ::Builder
  MQT::CoreNA
  MQT::QMapSCHeuristic
 )

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
