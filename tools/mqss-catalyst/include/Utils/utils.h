

#include "IR/QuantumOps.h"
using namespace mlir;
inline catalyst::quantum::CustomOp isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<catalyst::quantum::CustomOp>(op)) {
    return g;
    
  }
  return nullptr;
}

inline bool touchesQubit(mlir::Operation *op) {
    for (Type t : op->getOperandTypes())
        if (isa<catalyst::quantum::QubitType>(t))
            return true;

    for (Type t : op->getResultTypes())
        if (isa<catalyst::quantum::QubitType>(t))
            return true;

    return false;
}
