

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

inline void createCatalystGate(Location loc,
                               llvm::StringRef NewGateTy,
                               const std::vector<Value> TargetQubits,
                               mlir::IRRewriter &builder) {

    if (NewGateTy == "PauliZ" || NewGateTy == "PauliX" ||
        NewGateTy == "PauliY") {

        std::vector<mlir::Type> TargetQubitTys;
        for(auto t : TargetQubits){
          TargetQubitTys.push_back(t.getType());
        }
        auto newGate = builder.create<catalyst::quantum::CustomOp>(
            loc,
            /*out_qubits=*/mlir::TypeRange(TargetQubitTys),
            /*out_ctrl_qubits=*/mlir::TypeRange(),
            /*params=*/mlir::ValueRange(),
            /*in_qubits=*/mlir::ValueRange(TargetQubits),
            /*gate_name=*/NewGateTy,
            /*adjoint=*/false,
            /*in_ctrl_qubits=*/mlir::ValueRange(),
            /*in_ctrl_values=*/mlir::ValueRange());
    }
}
