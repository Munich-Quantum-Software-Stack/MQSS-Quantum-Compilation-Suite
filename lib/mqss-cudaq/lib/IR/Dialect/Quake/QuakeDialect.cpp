/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "IR/Dialect/Quake/QuakeDialect.h"
#include "IR/Dialect/Quake/QuakeOps.h"
#include "IR/Dialect/Quake/QuakeTypes.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/LoopLikeInterface.h"

//===----------------------------------------------------------------------===//
// Generated logic
//===----------------------------------------------------------------------===//

#include "IR/Dialect/Quake/QuakeDialect.cpp.inc"

//===----------------------------------------------------------------------===//

void quake::QuakeDialect::initialize() {
  registerTypes();
  addOperations<
#define GET_OP_LIST
#include "IR/Dialect/Quake/QuakeOps.cpp.inc"
      >();
  addInterfaces<QuakeInlinerInterface>();
}
