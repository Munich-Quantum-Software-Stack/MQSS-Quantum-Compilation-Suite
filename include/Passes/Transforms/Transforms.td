/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*************************************************************************
  author Akshay Bhosale
  co-author: Claude AI Sonnet/Opus
  date   August 2026
  version 2.0.0
*************************************************************************/

include "mlir/Pass/PassBase.td"


def CommonGateCancellationPass : Pass<"CommonGateCancellationPass", "mlir::ModuleOp"> {
  let summary = "Performs cancellation of Gates following a specific pattern";

  let description = [{
          This pass removes the pattern Gate, Gate if both gates operates on the same control and targets. 
          This pass can also remove the gates RX, RZ, RZ having NULL rotation angles.
  }];
                 
  let constructor = "mqss::opt::CommonGateCancellationPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"CancelGate\"", "Select Pattern to Cancel: CancelGate or CancelNullRotation.">,
  ];

}

def LLVMDialectToLLVMIRPass : Pass<"mlir-to-llvmIR", "mlir::ModuleOp"> {
  let summary = "Transforms LLVM dialect to LLVM IR";

  let description = [{
          Transforms LLVM dialect to LLVM IR.
  }];
                 
  let constructor = "mqss::opt::LLVMDialectToLLVMIRPass()";

}

def QuakeToQASM2Pass : Pass<"quake-to-qasm2", "mlir::ModuleOp"> {
  let summary = "Transforms Quake MLIR module to OpenQasm2";

  let description = [{
          Transforms Quake MLIR module to OpenQasm2.
  }];
                 
  let constructor = "mqss::opt::QuakeToQASM2Pass()";

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

   let constructor = "mqss::opt::QuantumConversionPass()";
   let dependentDialects = ["LLVM::LLVMDialect"];
}


def CommonCommutePass : Pass<"CommonCommutePass", "mlir::ModuleOp"> {
  let summary = "This pass searches commutes gates that match a specific pattern.";
  let constructor = "mqss::opt::CommonCommutePass()";

  let description = [{
          This pass searches for the gate Op pattern GATE1 followed by GATE2 and commutes them i.e
          GATE2 followed by GATE1. Supported Gate Op patterns are:
          1. CX followed by RX (Commuted to RX followed by CX)
          2. RX followed by CX
          3. CX followed by X
          4. X followed by CX
          5. CX followed by Z
          6. Z followed by CX
  }];

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"CX-RX\"", "Select Pattern to Commute: CX-RX, RX-CX, CX-X, X-CX, CX-Z, Z-CX.">,
  ];

}

def CommonSwitchPass : Pass<"CommonSwitchPass", "mlir::ModuleOp"> {
  let summary = "Commute and Switch Gates.";
 
  let description = [{
    Commute and Switch specific gates, Pauli{X,Y,Z}..H -> H...Pauli{X,Y,Z} 
    or H...Pauli{X,Y,Z} -> Pauli{X,Y,Z}..H.
  }];
 
 let constructor = "mqss::opt::CommonSwitchPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"XYZHtoHXYZ\"", "Select Pattern to Switch: XYZHtoHXYZ or HXYZtoXYZH.">,
  ];
}


def CommonReductionPass : Pass<"CommonReductionPass", "mlir::ModuleOp"> {
  let summary = "Perform Circuit Reduction, {HZH} -> X, {HXH} -> Z, {SAdjZ} -> S or {SZ} -> SAdj. ";
 
  let description = [{
    Perform Circuit Gate Pattern Reduction.
    Supported modes: {HZH} -> X, {HXH} -> Z, {SAdjZ} -> S or {SZ} -> SAdj.
  }];
 
 let constructor = "mqss::opt::CommonReductionPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"HXHToZ\"", "Select PassMode HXHToZ , HZHToX, SAdjZToS or SZToSAdj.">,
  ];
}

def CommonDecompositionPass : Pass<"CommonDecompositionPass", "mlir::ModuleOp"> {
  let summary = "Perform Gate Decomposition, {Cx} -> HCzH or {Cz} -> HCxH. ";
 
  let description = [{
        Perform Gate Decomposition, {Cx} -> HCzH or {Cz} -> HCxH. Also, can
        revers the control and targets of each
        "two-qubits CNot gate in a circuit
  }];
 
 let constructor = "mqss::opt::CommonDecompositionPass()";
  let dependentDialects = ["mlir::arith::ArithDialect"];
  
  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"CxToHCzH\"", "Select PassMode CxToHCzH ,CzToHCxH or ReverseCx.">,
  ];
}

def BasisConversionPass : Pass<"BasisConversionPass", "mlir::ModuleOp"> {
  let summary = "Recursively Decomposes to native-gate set of target device. ";
 
  let description = [{
        "Quick-and-dirty legalization pass: repeatedly rewrites any gate "
        "that isn't a member of the given native gate set, using the "
        "same rules as the individual Decomposition passes, until every "
        "gate is native or no further progress can be made"
  }];
 
 let constructor = "mqss::opt::BasisConversionPass()";
 let dependentDialects = ["mlir::arith::ArithDialect"];
 let options = [
    Option<"gates", "gates", "std::string",
      /*default=*//*default=*/"\"h,cx,rz\"", "Mnemonics of the gates considered native (e.g. h,cx,rz);.">,
  ];
}

def CommonCNOTReversePass : Pass<"CommonCNOTReversePass", "mlir::ModuleOp"> {
  let summary = "Reverse the control and targets of each CNot gate in a circuit";
 
  let description = [{
       Reverse the control and targets of each CNot gate in a circuit.
  }];
 
 let constructor = "mqss::opt::CommonCNOTReversePass()";
}

def CommonNormalizeArgAnglePass : Pass<"CommonNormalizeArgAnglePass", "mlir::ModuleOp"> {
  let summary = "Normalize Arg angle of RX, RY, RZ gates. ";
 
  let description = [{
    Normalize Arg angle of RX, RY, RZ gates.
  }];
 
 let constructor = "mqss::opt::CommonNormalizeArgAnglePass()";
}


def CommonMappingPass : Pass<"CommonMappingPass", "mlir::ModuleOp"> {
  let summary = "A dialect-agnostic mapping pass";
  let description = [{
    A dialect-agnostic mapping pass that accepts an input JSON
    and a QDMI device name.
  }];
  let options = [
    Option<"input", "input", "std::string",
           /*default=*/"\"\"",
           "Path to JSON input.">,
    Option<"qdmi", "qdmi", "std::string",
           /*default=*/"\"\"",
           "QDMI device name.">,
  ];

 let constructor = "mqss::opt::CommonMappingPass()";
}