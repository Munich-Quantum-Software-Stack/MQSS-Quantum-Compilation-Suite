

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

inline catalyst::quantum::CustomOp
createCatalystGate(Operation *OpToReplace, llvm::StringRef NewGateTy,
                   const SmallVector<mlir::Value, 2> ControlQubitsOps,
                   const SmallVector<mlir::Value, 2> TargetQubitOps,
                   const mlir::ValueRange params, mlir::IRRewriter &builder,
                   bool isAdj = false) {

  catalyst::quantum::CustomOp NewOp = nullptr;
 
  if (NewGateTy == "PauliZ" || NewGateTy == "PauliX" || NewGateTy == "PauliY" ||
      NewGateTy == "H" || NewGateTy == "RX" || NewGateTy == "RY" ||
      NewGateTy == "RZ") {

    std::vector<mlir::Type> TargetQubitTys;
    for (auto t : TargetQubitOps) {
      TargetQubitTys.push_back(t.getType());
    }
    NewOp = builder.create<catalyst::quantum::CustomOp>(
        OpToReplace->getLoc(),
        /*out_qubits=*/mlir::TypeRange(TargetQubitTys),
        /*out_ctrl_qubits=*/mlir::TypeRange(),
        /*params=*/params,
        /*in_qubits=*/mlir::ValueRange(TargetQubitOps),
        /*gate_name=*/NewGateTy,
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange(),
        /*in_ctrl_values=*/mlir::ValueRange());
    // TODO: Revisit this
    // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  }
  if (NewGateTy == "CNOT") {

    Value control = ControlQubitsOps[0];
    Value target = TargetQubitOps[0];

    NewOp = builder.create<catalyst::quantum::CustomOp>(
        OpToReplace->getLoc(),
        /*out_qubits=*/mlir::TypeRange{control.getType(), target.getType()},
        /*out_ctrl_qubits=*/mlir::TypeRange{},
        /*params=*/mlir::ValueRange{},
        /*in_qubits=*/mlir::ValueRange{control, target},
        /*gate_name=*/builder.getStringAttr("CNOT"), // or gateName if API
                                                     // accepts StringRef
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange{},
        /*in_ctrl_values=*/mlir::ValueRange{});

    // TODO: Revisit this
    // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
    // OpToReplace->getResult(1).replaceAllUsesWith(NewOp->getResult(1));
  }
  if(NewGateTy == "CZ"){
    llvm::outs() << "Building CZ...\n";
    Value control = ControlQubitsOps[0];
    Value target = TargetQubitOps[0];

    NewOp = builder.create<catalyst::quantum::CustomOp>(
        OpToReplace->getLoc(),
        /*out_qubits=*/mlir::TypeRange{control.getType(), target.getType()},
        /*out_ctrl_qubits=*/mlir::TypeRange{},
        /*params=*/mlir::ValueRange{},
        /*in_qubits=*/mlir::ValueRange{control, target},
        /*gate_name=*/builder.getStringAttr("CZ"), // or gateName if API
                                                     // accepts StringRef
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange{},
        /*in_ctrl_values=*/mlir::ValueRange{});
  }
  return NewOp;
}
