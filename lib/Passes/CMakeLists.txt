

add_mlir_library(MLIRPasses
  SHARED
  Transforms/CommonCNOTReversalPass.cpp
  Transforms/CommonGateCancellationPass.cpp
  Transforms/CommonGateCommutationPass.cpp
  Transforms/CommonCommuteAndSwitchPass.cpp
  Transforms/CommonDecompositionPass.cpp
  Transforms/CommonPatternReductionPass.cpp
  Transforms/CommonMappingPass.cpp
  Transforms/CommonNormalizeArgAnglePass.cpp
  Transforms/pipeline.cpp

  CodeGen/LLVMDialectToLLVMIR.cpp
  CodeGen/QuakeToQASM2.cpp
  CodeGen/ConversionPatterns.cpp
  CodeGen/QuantumToLLVMDialect.cpp
  CodeGen/BasisConversion.cpp
  CodeGen/GlobalizeArrayValues.cpp
  CodeGen/StaticAllocas.cpp

  DEPENDS
    MQSSTransformsPassIncGen

  LINK_LIBS PUBLIC
    qdmi::qdmi
    qdmi::example_driver
    CUDAQ::CC
    CUDAQ::Quake
    CUDAQ::QEC
    CATALYST::MBQC
    CATALYST::QRef
    CATALYST::Quantum
    MLIRFuncDialect         # <--- Fixes the func.func error
    MLIRArithDialect        # <--- For constants and math
    MLIRControlFlowDialect  # <--- For branching/returns
    MLIRLLVMDialect
    MLIRLinalgDialect
    MLIRIR
    MLIRPass
    MLIRTransforms

    MLIRLLVMCommonConversion
    MLIRBuiltinToLLVMIRTranslation
    MLIRLLVMToLLVMIRTranslation
    MLIRFuncInlinerExtension
    MLIRLLVMIRTransforms
    CUDAQ::CodeGen
    CUDAQ::Builder
    MQT::CoreIR
    MQT::CoreNA
    MQT::QMapSC
    MQT::QMapSCHeuristic


    # Conversions
    MLIRArithToLLVM
    MLIRComplexToLibm
    MLIRComplexToLLVM
    MLIRControlFlowToLLVM
    MLIRFuncToLLVM
    MLIRMathToFuncs
    MLIRMathToLLVM

    # Translation
    MLIRTargetLLVMIRExport
)

set_target_properties(MLIRPasses PROPERTIES
  BUILD_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
  INSTALL_RPATH "$<TARGET_FILE_DIR:qdmi::example_driver>"
)

target_compile_definitions(MLIRPasses PRIVATE
  ${MLIR_DEFINITIONS}
  ${LLVM_DEFINITIONS}
)
  
if(MQSS_ENABLE_DEBUG)
    target_compile_definitions(MLIRPasses PRIVATE
      MQSS_ENABLE_DEBUG
    )
endif()
target_compile_options(MLIRPasses PRIVATE -fexceptions)


target_include_directories(MLIRPasses
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
    ${QDMI_INCLUDE_DIR}
    ${MQT_QMAP_INCLUDE_DIR}
)


