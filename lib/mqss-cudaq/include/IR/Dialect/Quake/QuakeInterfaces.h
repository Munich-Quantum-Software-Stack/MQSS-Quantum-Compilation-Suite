/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#pragma once

#include "IR/Common/InlinerInterface.h"
#include "mlir/IR/OpDefinition.h"

using QuakeInlinerInterface = cudaq::EnableInlinerInterface;

//===----------------------------------------------------------------------===//
// Generated logic
//===----------------------------------------------------------------------===//

#include "IR/Dialect/Quake/QuakeInterfaces.h.inc"
