/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"


inline std::tuple<bool, llvm::StringLiteral> isQuakeQuantumGate(mlir::Operation *op) {
  if (auto x = dyn_cast<quake::XOp>(op))
    return {true, "CNOT"};

  if(auto x = dyn_cast<quake::RxOp>(op))
    return {true, "RX"};

  if(auto x = dyn_cast<quake::HOp>(op))
    return {true, "H"};

  if(auto x = dyn_cast<quake::ZOp>(op))
    return {true, "Z"};

  if(auto x = dyn_cast<quake::YOp>(op))
    return {true, "Y"};
  
  return {false, ""};
}