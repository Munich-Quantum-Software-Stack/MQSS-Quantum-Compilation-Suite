

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
      /*default=*//*default=*/"\"CXRX\"", "Select Pattern to Commute: CX-RX, RX-CX, CX-X, X-CX, CX-Z, Z-CX.">,
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
  let summary = "Perform Circuit Reduction, {HZH} -> X or {HXH} -> Z. ";
 
  let description = [{
    Perform Circuit Reduction, {HZH} -> X or {HXH} -> Z.
  }];
 
 let constructor = "mqss_catalyst::opt::CommonReductionPass()";

  let options = [
    Option<"mode", "mode", "std::string",
      /*default=*//*default=*/"\"HXHToZ\"", "Select PassMode HXHToZ or HZHToX.">,
  ];
}