add_mlir_library(
  MQSSCICodeGenPasses
  LLVMDialectToLLVMIRPass.cpp
  QuakeToQASM2Pass.cpp
  ConversionPatterns.cpp
  QuantumToLLVMDialectPass.cpp
  BasisConversionPass.cpp
  GlobalizeArrayValuesPass.cpp
  StaticAllocas.cpp
  DEPENDS
  MQSSMQSSCodeGenPassIncGen
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
