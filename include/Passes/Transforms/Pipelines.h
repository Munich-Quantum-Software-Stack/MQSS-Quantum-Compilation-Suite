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
/** @file
 * @brief
 * @details This header defines the three optimization levels supported by the
 * MQSS.
 * @details This header defines the three optimization levels supported by the
 * MQSS: `O1`, `O2` and `O3`. Each function appends the corresponding list of
 * optimization passes to a given `mlir::PassManager` object.
 *
 * @par
 * This header must be included to use the different optimization levels that
 * are part of the MQSS.
 */

#pragma once

#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

namespace mqss::opt {
/**
  @brief Function defining the optimization level `O1`.
  @details This functions appends to a `mlir::PassManager` the list of passes
  corresponding to optimization level `O1`.
  @param[out] pm is the `mlir::PassManager` after appending the list of passes
  corresponding to optimization level `O1`.
*/
void O1(mlir::OpPassManager &pm);
/**
  @brief Function defining the optimization level `O2`.
  @details This functions appends to a `mlir::PassManager` the list of passes
  corresponding to optimization level `O2`.
  @param[out] pm is the `mlir::PassManager` after appending the list of passes
  corresponding to optimization level `O2`.
*/
void O2(mlir::OpPassManager &pm);
/**
  @brief Function defining the optimization level `O3`.
  @details This functions appends to a `mlir::PassManager` the list of passes
  corresponding to optimization level `O3`.
  @param[out] pm is the `mlir::PassManager` after appending the list of passes
  corresponding to optimization level `O3`.
*/
void O3(mlir::OpPassManager &pm);
void QIRConversionPipeline(mlir::OpPassManager &pm,
                           const std::string &qir_version,
                           llvm::raw_ostream &os = llvm::outs());
} // namespace mqss::opt
