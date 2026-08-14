add_mlir_dialect_library(
  MQSSSupportedDialects
  Dialects.cpp
  LINK_LIBS
  PUBLIC
  CUDAQ::CC
  CUDAQ::Quake
  CUDAQ::QEC
  CATALYST::MBQC
  CATALYST::QRef
  CATALYST::Quantum
  MLIRFuncDialect # <--- Fixes the func.func error
  MLIRArithDialect # <--- For constants and math
  MLIRControlFlowDialect # <--- For branching/returns
  MLIRLLVMDialect
  MLIRLinalgDialect
  MLIRIR
  MLIRLLVMCommonConversion
  MLIRBuiltinToLLVMIRTranslation
  MLIRLLVMToLLVMIRTranslation
  MLIRFuncInlinerExtension
  MLIRLLVMIRTransforms
  # Conversions
  MLIRArithToLLVM
  MLIRComplexToLibm
  MLIRComplexToLLVM
  MLIRControlFlowToLLVM
  MLIRFuncToLLVM
  MLIRMathToFuncs
  MLIRMathToLLVM
  # Translation
  MLIRTargetLLVMIRExport)
