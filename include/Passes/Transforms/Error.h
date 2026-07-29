/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*************************************************************************
  author Akshay Bhosale
  co-author: Claude AI Sonnet/Opus
  date   August 2026
  version 2.0.0
*************************************************************************/

#pragma once

#include "mlir/IR/Diagnostics.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

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

} // namespace mqss::opt
