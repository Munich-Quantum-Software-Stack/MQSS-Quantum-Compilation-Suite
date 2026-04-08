/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"

using namespace llvm;
using namespace mlir;

inline std::tuple<bool, StringLiteral>

isQuakeQuantumGate(Operation *op) {
  
  if (auto x = dyn_cast<quake::XOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliX"};
    return {true, "CNOT"};
  }

  if (auto x = dyn_cast<quake::RxOp>(op))
    return {true, "RX"};
  if (auto x = dyn_cast<quake::RyOp>(op))
    return {true, "RY"};
  if (auto x = dyn_cast<quake::RzOp>(op))
    return {true, "RZ"};

  if (auto x = dyn_cast<quake::HOp>(op))
    return {true, "H"};

  if (auto x = dyn_cast<quake::ZOp>(op))
    return {true, "PauliZ"};

  if (auto x = dyn_cast<quake::YOp>(op))
    return {true, "PauliY"};

  return {false, ""};
}

inline void createQuakeGate(Location loc, llvm::StringRef NewGateTy,
                       const std::vector<Value> TargetQubits,
                       mlir::IRRewriter &builder) {


  if (NewGateTy == "PauliZ") {
        builder.create<quake::ZOp>(loc, false, mlir::ValueRange(),
                                    mlir::ValueRange(), TargetQubits);
  }
  else if (NewGateTy == "PauliX") {
    auto newGate =
        builder.create<quake::XOp>(loc, false, mlir::ValueRange(),
                                    mlir::ValueRange(), TargetQubits);
  }
    else if (NewGateTy == "PauliY") {
    auto newGate =
        builder.create<quake::YOp>(loc, false, mlir::ValueRange(),
                                    mlir::ValueRange(), TargetQubits);
  }
}
