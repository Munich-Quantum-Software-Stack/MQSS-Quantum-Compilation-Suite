include "mlir/Pass/PassBase.td"


def CommonCxCancellationPass : Pass<"CommonCxCancellationPass"> {
  let summary = "This pass removes the pattern CNot, CNot if both gates operates on the same control and targets. ";
  let constructor = "mqss_cudaq::opt::CommonCxCancellationPass()";
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


def CommonSwitchHXPass : Pass<"CommonSwitchHXPass"> {
  let summary = "This pass searches for the gate Op pattern Hadamard followed by "
                "PauliX and switched it to PauliZ followed by Hadamard";
  let constructor = "mqss_cudaq::opt::CommonSwitchHXPass()";
}

def CommonSwitchHYPass : Pass<"CommonSwitchHYPass"> {
  let summary = "This pass searches for the gate Op pattern - Hadamard followed by "
                "PauliY and switches it to PauliY followed by "
                "Hadamard";
  let constructor = "mqss_cudaq::opt::CommonSwitchHYPass()";
}

def CommonSwitchXHPass : Pass<"CommonSwitchXHPass"> {
  let summary = "This pass searches for the gate Op pattern - PauliX followed by "
                "Hadamard and switches it to Hadamard followed by "
                "PauliZ";
  let constructor = "mqss_cudaq::opt::CommonSwitchXHPass()";
}

def CommonSwitchHZPass : Pass<"CommonSwitchHZPass"> {
  let summary = "This pass searches for the gate Op pattern Hadamard followed by "
                "PauliZ and switched it to PauliX followed by Hadamard";
  let constructor = "mqss_cudaq::opt::CommonSwitchHZPass()";
}
