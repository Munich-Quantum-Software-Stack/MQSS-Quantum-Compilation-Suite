/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"

inline mlir::Operation *isQuakeGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<quake::XOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::HOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::YOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::ZOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::R1Op>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::RxOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::RyOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::RzOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::SOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::TOp>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::U2Op>(op)) {
    return g;
  }
  if (auto g = llvm::dyn_cast<quake::SwapOp>(op)) {
    return g;
  }
  return nullptr;
}