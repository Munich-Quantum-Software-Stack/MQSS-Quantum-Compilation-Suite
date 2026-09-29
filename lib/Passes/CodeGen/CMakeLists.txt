add_mlir_library(
  MQSSCICodeGenPasses
  SHARED
  LLVMDialectToLLVMIRPass.cpp
  QuakeToQASM2Pass.cpp
  ConversionPatterns.cpp
  QuantumToLLVMDialectPass.cpp
  BasisConversionPass.cpp
  GlobalizeArrayValuesPass.cpp
  StaticAllocas.cpp
  TranslateToIQMJSON.cpp
  DEPENDS
  MQSSMQSSCodeGenPassIncGen
  MQSSTransformsPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  CUDAQ::CodeGen
  CUDAQ::Builder
  qdmi::qdmi
  qdmi::example_driver
  MLIRPass
  MLIRTransforms
  MLIRFuncToLLVM
  MLIRMathToFuncs
  MLIRMathToLLVM
  # Translation
  MLIRTargetLLVMIRExport)

target_compile_options(MQSSCICodeGenPasses PRIVATE -fexceptions)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(MQSSCICodeGenPasses PRIVATE MQSS_ENABLE_DEBUG)
endif()

# Needed so MQT-QMap's sc/utils.hpp can find MQT-Core's OpType.hpp. This header
# is pulled in by TranspilationPassUtils.h.
target_include_directories(MQSSCICodeGenPasses
                           PRIVATE ${MQT_CORE_INCLUDE_DIR}/mqt-core)
