

#include "IR/QuantumOps.h"

bool isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<catalyst::quantum::CustomOp>(op)) {
    return true;
  }
  return false;
}