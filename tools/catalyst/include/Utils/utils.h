

#include "IR/QuantumOps.h"

inline bool isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<catalyst::quantum::CustomOp>(op)) {
    return true;
  }
  return false;
}