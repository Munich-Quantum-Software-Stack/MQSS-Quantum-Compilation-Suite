include "mlir/Pass/PassBase.td"


def CommonGateCancellationPass : Pass<"CommonGateCancellationPass"> {
  let summary = "This pass removes the pattern Gate, Gate if both gates operates on the same control and targets. ";
  let constructor = "mqss_cudaq::opt::CommonGateCancellationPass()";
}

def CommonNullRotationCancellationPass : Pass<"CommonNullRotationCancellationPass"> {
  let summary = "TThis pass removes the gates RX, RZ, RZ having NULL rotation angles.";
  let constructor = "mqss_cudaq::opt::CommonNullRotationCancellationPass()";
}

def CommonCommuteCxRxPass : Pass<"CommonCommuteCxRxPass"> {
  let summary = "This pass searches for the gate Op pattern CNOT followed by Rx and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteCxRxPass()";
}

def CommonCommuteCxXPass : Pass<"CommonCommuteCxXPass"> {
  let summary = "This pass searches for the gate Op pattern CNOT followed by PAULIX and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteCxXPass()";
}

def CommonCommuteXCxPass : Pass<"CommonCommuteXCxPass"> {
  let summary = "This pass searches for the gate Op pattern PAULIX followed by CNOT and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteXCxPass()";
}

def CommonCommuteCxZPass : Pass<"CommonCommuteCxZPass"> {
  let summary = "This pass searches for the gate Op pattern CNOT followed by PAULIZ and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteCxZPass()";
}

def CommonCommuteZCxPass : Pass<"CommonCommuteZCxPass"> {
  let summary = "This pass searches for the gate Op pattern PAULIZ followed by CNOT and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteZCxPass()";
}

def CommonCommuteRxCxPass : Pass<"CommonCommuteRxCxPass"> {
  let summary = "This pass searches for the gate Op pattern Rx followed by CNOT and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteRxCxPass()";
}

def CommonSwitchHXYZPass : Pass<"CommonSwitchHXYZPass"> {
  let summary = "This pass searches for the gate Op pattern - Hadamard followed by "
                "Pauli{X,Y,Z} gate and switches it to Pauli{X,Y,Z} followed by "
                "Hadamard";
  let constructor = "mqss_cudaq::opt::CommonSwitchHXYZPass()";
}

def CommonSwitchXYZHPass : Pass<"CommonSwitchXYZHPass"> {
  let summary = "This pass searches for the gate Op pattern - Pauli{X,Y,Z} followed by "
                "Hadamard gate and switches it to Hadamard followed by "
                "Pauli{X,Y,Z}";
  let constructor = "mqss_cudaq::opt::CommonSwitchXYZHPass()";
}