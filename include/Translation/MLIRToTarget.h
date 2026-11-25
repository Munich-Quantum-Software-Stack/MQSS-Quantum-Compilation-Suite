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
 * @brief Translations that emit an MLIR module as a target text format.
 * @details These are the "from MLIR" counterparts of the `--import-qasm2`
 * translation. Each one runs the matching MQSS lowering pipeline on the module
 * and streams the result to mlir-translate's output, so the pipelines are
 * reachable from the `mqss-translate` command line.
 */

#pragma once

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Pipelines/Pipelines.h"
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/TransformPasses.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Tools/mlir-translate/Translation.h"

namespace mqss::mqssci::translation {

using PipelineBuilder =
    std::function<void(OpPassManager &, llvm::raw_ostream &)>;

// Builds a pipeline with `build` and runs it on `op`. The last pass of every
// pipeline writes its text output to `os`, which is mlir-translate's output.
LogicalResult runPipeline(Operation *op, llvm::raw_ostream &os,
                          const PipelineBuilder &build);

TranslateFromMLIRFunction makeTranslation(PipelineBuilder build);

// For pipelines whose result is an MLIR module rather than text.
TranslateFromMLIRFunction makeModuleTranslation(PipelineBuilder build);

// The input module is parsed by mlir-translate, so every dialect the MQSS
// pipelines can consume must be registered.
void registerDialects(DialectRegistry &registry);

// `profile` is "<name>:<version>", e.g. "qir-base:2.0". It is validated when
// the translation runs so a bad value is reported as a diagnostic.
mlir::TranslateFromMLIRFunction makeQIRTranslation(std::string profile);

/**
  @brief Registers the MLIR-to-target translations with mlir-translate.
  @details Registers `--mlir-to-qasm2`, `--mlir-to-qasm3`,
  `--mlir-to-iqm-json`, `--mlir-to-qir`, `--mlir-to-qir-base`,
  `--mlir-to-qir-adaptive` and `--mlir-to-qir-full` (QIR version 2.0).
  The input module is expected to already be lowered to the target's gate set
  (e.g. by `mqss-opt --BasisConversionPass`).
*/
void registerMLIRToTargetTranslations();

} // namespace mqss::mqssci::translation
