
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"

bool isQuakeGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<quake::XOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::HOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::YOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::ZOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::R1Op>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::RxOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::RyOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::RzOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::SOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::TOp>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::U2Op>(op)) {
    return true;
  } else if (auto g = llvm::dyn_cast<quake::SwapOp>(op)) {
    return true;
  } else {
    return false;
  }
}