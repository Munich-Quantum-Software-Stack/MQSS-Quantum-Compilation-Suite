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
