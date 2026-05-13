

include "mlir/Pass/PassBase.td"

def CommonGateCancellationPass : Pass<"CommonGateCancellationPass", "mlir::ModuleOp"> {
  let summary = "Performs cancellation of Gates following a specific pattern";

  let description = [{
          This pass removes the pattern Gate, Gate if both gates operates on the same control and targets. 
          This pass can also remove the gates RX, RZ, RZ having NULL rotation angles.
  }];
                 
  let constructor = "mqss_catalyst::opt::CommonGateCancellationPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"CancelGate\"", "Select Pattern to Cancel: CancelGate or CancelNullRotation.">,
  ];

}

def CommonCommutePass : Pass<"CommonCommutePass", "mlir::ModuleOp"> {
  let summary = "This pass searches commutes gates that match a specific pattern.";
  let constructor = "mqss_catalyst::opt::CommonCommutePass()";

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
 
 let constructor = "mqss_catalyst::opt::CommonSwitchPass()";

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
 
 let constructor = "mqss_catalyst::opt::CommonReductionPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"HXHToZ\"", "Select PassMode HXHToZ , HZHToX, SAdjZToS or SZToSAdj.">,
  ];
}

def CommonDecompositionPass : Pass<"CommonDecompositionPass", "mlir::ModuleOp"> {
  let summary = "Perform Gate Decomposition, {Cx} -> HCzH or {Cz} -> HCxH or ReverseCx. ";
 
  let description = [{
        Perform Gate Decomposition, {Cx} -> HCzH or {Cz} -> HCxH. Also, can
        revers the control and targets of each
        "two-qubits CNot gate in a circuit
  }];
 
 let constructor = "mqss_catalyst::opt::CommonDecompositionPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"CxToHCzH\"", "Select PassMode CxToHCzH ,CzToHCxH or ReverseCx.">,
  ];
}


def CommonNormalizeArgAnglePass : Pass<"CommonNormalizeArgAnglePass", "mlir::ModuleOp"> {
  let summary = "Normalize Arg angle of RX, RY, RZ gates. ";
 
  let description = [{
    Normalize Arg angle of RX, RY, RZ gates.
  }];
 
 let constructor = "mqss_catalyst::opt::CommonNormalizeArgAnglePass()";
}

def CommonMappingPass : Pass<"CommonMappingPass", "mlir::ModuleOp"> {
  let summary = "A Dialect agnostic mapping pass";
 
  let description = [{
    A Dialect agnostic mapping pass
  }];
 
 let constructor = "mqss_catalyst::opt::CommonMappingPass()";
}