/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#pragma once

#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "mlir/IR/Diagnostics.h"
#include <cstdlib>

namespace mqss::opt {

/// The emitFatalError() function is used when the compiler reaches a point that
/// it cannot continue and produce valid output code. This is very much like an
/// assertion, but it will not be removed if assertions are disabled.
[[noreturn]] inline void MQSSemitFatalError(mlir::Location loc,
                                        const llvm::Twine &message) {
  mlir::emitError(loc, message);
  llvm::report_fatal_error("fatal error, aborting.");
}

} // namespace mqss
