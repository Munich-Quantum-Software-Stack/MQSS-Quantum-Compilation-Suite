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
*/

/** @file
 * @brief OpenQASM 2 -> CUDA-Q Quake translation.
 * @details This is a translation (text in, module out) rather than an MLIR
 * pass, since a QASM2 string is not an MLIR module a pass could run on. It is
 * the inverse direction of the QuakeToQASM2Pass.
 */

#pragma once

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

#include "llvm/ADT/StringRef.h"

namespace mqss::mqssci::translation {

// Translates OpenQASM 2 source text into a module in the CUDA-Q Quake dialect.
// Diagnostics (parse errors, unsupported gates) are emitted through `context`.
// @param qasm OpenQASM 2 program text.
// @param context Context with the MQSS dialects registered.
// @return The translated module, or a null OwningOpRef on failure.
mlir::OwningOpRef<mlir::ModuleOp>
translateQASM2ToQuake(llvm::StringRef qasm, mlir::MLIRContext &context);

// Registers the `--import-qasm2` translation with mlir-translate's registry.
void registerQASM2ToQuakeTranslation();

} // namespace mqss::mqssci::translation
