include "mlir/Pass/PassBase.td"

def CommuteCxRx : Pass<"CommuteCxRx"> {
  let summary = "Apply commutation pass of pattern CNot-Rx";
  let constructor = "mqss_cudaq::opt::createCommuteCxRxPass()";
}

// def  CommuteCxX : Pass<"CommuteCxX"> {
//   let summary = "Apply commutation pass to pattern CNot-X";
//   let constructor = "mqss::opt::createCommuteCxXPass()";
// }

// def CommuteCxZ : Pass<"CommuteCxZ"> {
//   let summary = "Apply commutation pass to pattern CNot-Z to Z-CNot";
//   let constructor = "mqss::opt::createCommuteCxZPass()";
// }

// def CommuteRxCx : Pass<"CommuteRxCx"> {
//   let summary = "Apply commutation pass to pattern Rx-CNot to CNot-Rx";
//   let constructor = "mqss::opt::createCommuteRxCxPass()";
// }

// def CommuteXCx : Pass<"CommuteXCx"> {
//   let summary = "Apply commutation pass to pattern X-CNot to CNot-X";
//   let constructor = "mqss::opt::createCommuteXCxPass()";
// }

// def CommuteZCx : Pass<"CommuteZCx"> {
//   let summary = "Apply commutation pass to pattern Z-CNot to CNot-Z";
//   let constructor = "mqss::opt::createCommuteZCxPass()";
// }

def CancellationDoubleCx : Pass<"CancellationDoubleCx"> {
  let summary = "This pass removes the pattern CNot, CNot if both gates operates on the same control and targets. ";
  let constructor = "mqss_cudaq::opt::createCancellationDoubleCxPass()";
}

// def ReductionPattern : Pass<"ReductionPattern"> {
//   let summary = "This pass removes the pattern Gate, Gate if both gates operates on the same control and targets. ";
//   let constructor = "mqss::opt::createReductionPatternPass()";
// }

// def HXHToZ : Pass<"HXHToZ"> {
//   let summary = " Optimization pass that replaces a pattern composed of H, X, H by Z";
//   let constructor = "mqss::opt::createHXHToZPass()";
// }

// def HZHToX : Pass<"HZHToX"> {
//   let summary = "Optimization pass that replaces a pattern composed of H, Z, H by X";
//   let constructor = "mqss::opt::createHZHToXPass()";
// }

// def SwitchHX : Pass<"SwitchHX"> {
//   let summary = "Pass that switches a pattern composed Hadamard and X to Z and Hadamard";
//   let constructor = "mqss::opt::createSwitchHXPass()";
// }

// def SwitchHY : Pass<"SwitchHY"> {
//   let summary = "Pass that switches a pattern composed Hadamard and Y to Y and Hadamard";
//   let constructor = "mqss::opt::createSwitchHYPass()";
// }

// def SwitchHZ : Pass<"SwitchHZ"> {
//   let summary = "Pass that switches a pattern composed Hadamard and Z to X and Hadamard";
//   let constructor = "mqss::opt::createSwitchHZPass()";
// }

// def NormalizeArgAngle : Pass<"NormalizeArgAngle"> {
//   let summary = "Optimization pass that normalizes the angle of Rx, Ry and Rz rotations";
//   let constructor = "mqss::opt::createNormalizeArgAnglePass()";
// }

// def CancellationNullRotation : Pass<"CancellationNullRotation"> {
//   let summary = "Optimization pass that removes of Rx, Ry and Rz null rotations";
//   let constructor = "mqss::opt::createCancellationNullRotationPass()";
// }

// def SwitchPauliH : Pass<"SwitchPauliH"> {
//   let summary = "Pass that switches a pattern composed by {X,Y,Z} (Pauli) and Hadamard";
//   let constructor = "mqss::opt::createSwitchPauliHPass()";
// }

// def SwitchXH : Pass<"SwitchXH"> {
//   let summary = "Pass that switches a pattern composed by X and Hadamard to Hadamard and Z";
//   let constructor = "mqss::opt::createSwitchXHPass()";
// }

// def SwitchYH : Pass<"SwitchYH"> {
//   let summary = "Pass that switches a pattern composed by Y and Hadamard to Hadamard and Y";
//   let constructor = "mqss::opt::createSwitchYHPass()";
// }

// def SwitchZH : Pass<"SwitchZH"> {
//   let summary = "Pass that switches a pattern composed by Z and Hadamard to Hadamard and X";
//   let constructor = "mqss::opt::createSwitchZHPass()";
// }
