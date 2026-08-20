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
 * @brief
 * @details Header file that defines the signature for each MLIR/Quake defined
 * into the Munich Quantum Software Stack (MQSS).
 *
 * @par
 * This header must be included to use the collection of transforms passes that
 * are part of the MQSS.
 */

#pragma once

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"

#include "llvm/Support/raw_ostream.h"

#include <stdexcept>

#define GEN_PASS_DECL
#include "Passes/CodeGen/CodeGen.h.inc"
#include "Utils/CodegenUtils.h"

namespace mqssci::codegen {

std::unique_ptr<mlir::Pass> LLVMDialectToLLVMIRPass();
std::unique_ptr<mlir::Pass> QuakeToQASM2Pass();
std::unique_ptr<mlir::Pass> QuantumConversionPass();
std::unique_ptr<mlir::Pass> BasisConversionPass();
std::unique_ptr<mlir::Pass>
createBasisConversionPass(const BasisConversionPassOptions &options);
std::unique_ptr<mlir::Pass> LLVMDialectToLLVMIRPass(llvm::raw_ostream &os);

std::unique_ptr<mlir::Pass> QuakeToQASM2Pass(llvm::raw_ostream &os);

} // namespace mqssci::codegen

#define GEN_PASS_REGISTRATION
#include "Passes/CodeGen/CodeGen.h.inc"
