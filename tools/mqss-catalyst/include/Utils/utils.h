

#include "IR/QuantumOps.h"


using namespace mlir;


inline catalyst::quantum::CustomOp
isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<catalyst::quantum::CustomOp>(op)) {
    return g;
  }
  return nullptr;
}

inline bool hasQuantumEffect(Operation *op) {
    for (auto type : op->getOperandTypes()) {
        if (isa<catalyst::quantum::QubitType>(type))
            return true;
    }
    for (auto type : op->getResultTypes()) {
        if (isa<catalyst::quantum::QubitType>(type))
            return true;
    }
    return false;
}

inline void createGate(mlir::Operation *SwitchOutGate,
                               llvm::StringRef NewGateTy,
                               const Value TargetQubit,
                               mlir::IRRewriter &builder) {

  if (NewGateTy == "PauliZ" || NewGateTy == "PauliX") {

    auto newGate =
        builder.create<catalyst::quantum::CustomOp>(
            SwitchOutGate->getLoc(),
            /*out_qubits=*/mlir::TypeRange({TargetQubit.getType()}),
            /*out_ctrl_qubits=*/mlir::TypeRange(),
            /*params=*/mlir::ValueRange(),
            /*in_qubits=*/mlir::ValueRange({TargetQubit}),
            /*gate_name=*/NewGateTy,
            /*adjoint=*/false,
            /*in_ctrl_qubits=*/mlir::ValueRange(),
            /*in_ctrl_values=*/mlir::ValueRange());
  }
}
