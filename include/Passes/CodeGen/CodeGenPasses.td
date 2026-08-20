
include "mlir/Pass/PassBase.td"

def LLVMDialectToLLVMIRPass : Pass<"mlir-to-llvmIR", "mlir::ModuleOp"> {
  let summary = "Transforms LLVM dialect to LLVM IR";

  let description = [{
          Transforms LLVM dialect to LLVM IR.
  }];

  let constructor = "mqss::mqssci::codegen::LLVMDialectToLLVMIRPass()";

}

def QuakeToQASM2Pass : Pass<"quake-to-qasm2", "mlir::ModuleOp"> {
  let summary = "Transforms Quake MLIR module to OpenQasm2";

  let description = [{
          Transforms Quake MLIR module to OpenQasm2.
  }];

  let constructor = "mqss::mqssci::codegen::QuakeToQASM2Pass()";

}

def QuantumConversionPass : Pass<"convert-quantum-to-llvm"> {
    let summary = "Perform a dialect conversion from Quantum dialect to LLVM dialect.";

    let options = [
        Option<
            "useArrayBackedRegisters",
            "use-array-backed-registers",
            "bool",
            default="false",
            desc="Use the array-backed-registers conversion pattern for quantum.insert ops."
        >
    ];

   let constructor = "mqss::mqssci::codegen::QuantumConversionPass()";
   let dependentDialects = ["LLVM::LLVMDialect"];
}

def BasisConversionPass : Pass<"BasisConversionPass", "mlir::ModuleOp"> {
  let summary = "Recursively Decomposes to native-gate set of target device. ";

  let description = [{
        "Quick-and-dirty legalization pass: repeatedly rewrites any gate "
        "that isn't a member of the given native gate set, using the "
        "same rules as the individual Decomposition passes, until every "
        "gate is native or no further progress can be made"
  }];

 let constructor = "mqss::mqssci::codegen::BasisConversionPass()";
 let dependentDialects = ["mlir::arith::ArithDialect"];
 let options = [
    Option<"gates", "gates", "std::string",
      /*default=*//*default=*/"\"h,cx,rz\"", "Mnemonics of the gates considered native (e.g. h,cx,rz);.">,
  ];
}
