/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"

inline mlir::Operation *isQuakeGateOp(mlir::Operation *Op) {

  if (auto g = dyn_cast<quake::OperatorInterface>(Op)) {
  }
  return nullptr;
}
inline bool isQuakeQuantumGate(mlir::Operation *Op) {
  return Op->hasTrait<cudaq::QuantumGate>();
}