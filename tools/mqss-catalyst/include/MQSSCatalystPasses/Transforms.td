

include "mlir/Pass/PassBase.td"

def GateCancellationPass : Pass<"MyGateCancellationPass"> {
  let summary = "This pass removes the pattern CNot, CNot if both gates operates on the same control and targets. ";
  let constructor = "catalyst::opt::GateCancellationPass()";
}

def GateCommutationPass : Pass<"MyGateCommutationPass"> {
  let summary = "This pass searches for a specific pattern T1 followed by T2 and commutes them.";
  let constructor = "catalyst::opt::GateCommutationPass()";
}