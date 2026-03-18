

#include "IR/QuantumOps.h"


using namespace mlir;


inline catalyst::quantum::CustomOp
isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<catalyst::quantum::CustomOp>(op)) {
    return g;
  }
  return nullptr;
}

inline void createGate(mlir::Operation *SwitchOutGate,
                               llvm::StringRef NewGateTy,
                               const Value TargetQubit,
                               mlir::IRRewriter &builder) {

  if (NewGateTy == "PauliZ") {

    auto newGate =
        builder.create<catalyst::quantum::CustomOp>(
            SwitchOutGate->getLoc(),
            /*out_qubits=*/mlir::TypeRange({TargetQubit.getType()}),
            /*out_ctrl_qubits=*/mlir::TypeRange(),
            /*params=*/mlir::ValueRange(),
            /*in_qubits=*/mlir::ValueRange({TargetQubit}),
            /*gate_name=*/"PauliZ",
            /*adjoint=*/false,
            /*in_ctrl_qubits=*/mlir::ValueRange(),
            /*in_ctrl_values=*/mlir::ValueRange());
  }
}
