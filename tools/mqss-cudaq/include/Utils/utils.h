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

inline std::tuple<bool, llvm::StringLiteral>
isQuakeQuantumGate(mlir::Operation *op) {
  if (auto x = dyn_cast<quake::XOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliX"};
    return {true, "CNOT"};
  }

  if (auto x = dyn_cast<quake::RxOp>(op))
    return {true, "RX"};

  if (auto x = dyn_cast<quake::HOp>(op))
    return {true, "H"};

  if (auto x = dyn_cast<quake::ZOp>(op))
    return {true, "Z"};

  if (auto x = dyn_cast<quake::YOp>(op))
    return {true, "Y"};

  return {false, ""};
}

inline void createAndEraseGate(mlir::Operation *SwitchOutGate,
                               llvm::StringRef NewGateTy) {
  mlir::IRRewriter rewriter(SwitchOutGate->getContext());
  rewriter.setInsertionPointAfter(SwitchOutGate);

  auto gate = dyn_cast<quake::OperatorInterface>(SwitchOutGate);

  quake::OperatorInterface OpInterface;

  if (NewGateTy == "PauliZ") {
    auto newGate =
        rewriter.create<quake::ZOp>(gate.getLoc(), false, mlir::ValueRange(),
                                    mlir::ValueRange(), gate.getTargets());
  }

  rewriter.eraseOp(SwitchOutGate);
}
