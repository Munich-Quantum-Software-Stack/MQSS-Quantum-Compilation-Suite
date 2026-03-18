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

def CommonCommuteCxZPass : Pass<"CommonCommuteCxzPass"> {
  let summary = "This pass searches for the gate Op pattern CNOT followed by PAULIZ and commutes them.";
  let constructor = "mqss_cudaq::opt::CommonCommuteCxZPass()";
}

def CommonSwitchHXPass : Pass<"CommonSwitchHXPass"> {
  let summary = "This pass searches for the gate Op pattern Hadamard followed by "
                "X and switched it to Z followed by Hadamard";
  let constructor = "mqss_cudaq::opt::CommonSwitchHXPass()";
}
