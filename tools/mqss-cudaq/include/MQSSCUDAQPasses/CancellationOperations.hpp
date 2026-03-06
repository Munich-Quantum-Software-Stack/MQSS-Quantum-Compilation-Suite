/* This code and any associated documentation is provided "as is"

Copyright 2025 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*******************************************************************************
  author Martin Letras
  date   February 2025
  version 1.0
*******************************************************************************
* This source code and the accompanying materials are made available under    *
* the terms of the Apache License 2.0 which accompanies this distribution.    *
******************************************************************************/

/** @file
    @brief
    @details Definition of the cancellation operation at MLIR level. Given a
   pattern specified by the template, the function will find the pattern and
   remove it from a given mlir module.
    @par
    This header must be included to use the pattern cancellation into a pass.
*/

#pragma once

#include "../Utils/Quake.hpp"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mqss_cudaq::support::quakeDialect;

namespace mqss_cudaq::support::transforms {

/**
 * @brief Function that removes (cancel) a pattern of two quantum operations
 * under specific constraints.
 * @details This function examines the current operation and tries to cancel two
 * operations based on the number of control and target qubits they involve.
 *
 * @tparam T1 is the type of the first operation (e.g., a specific MLIR Op
 * class).
 * @tparam T2 is the type of the second operation.
 * @param[in] currentOp pointer to the current MLIR operation being analyzed.
 * @param[in] nCtrlsOp1 number of control qubits in the first operation.
 * @param[in] nTgtsOp1 number of target qubits in the first operation.
 * @param[in] nCtrlsOp2 number of control qubits in the second operation.
 * @param[in] nTgtsOp2 number of target qubits in the second operation.
 */
template <typename T1, typename T2>
void patternCancellation(mlir::Operation *currentOp, int nCtrlsOp1,
                         int nTgtsOp1, int nCtrlsOp2, int nTgtsOp2) {
  auto currentGate = dyn_cast_or_null<T2>(*currentOp);
  if (!currentGate)
    return;
  // check that the current gate is compliant with the number of controls and
  // targets
  if (currentGate.getControls().size() != nCtrlsOp2 ||
      currentGate.getTargets().size() != nTgtsOp2)
    return;
  // get the previous operation to check the swap pattern
  auto prevOp =
      getPreviousOperationOnTarget(currentGate, currentGate.getTargets()[0]);
  if (!prevOp)
    return;
  auto previousGate = dyn_cast_or_null<T1>(prevOp);
  if (!previousGate)
    return;
  // check that the previous gate is compliant with the number of controls and
  // targets
  if (previousGate.getControls().size() != nCtrlsOp1 ||
      previousGate.getTargets().size() != nTgtsOp1)
    return;
  // check that targets and controls are the same!
  // At the moment I am checking all controls and all targets!
  if (currentGate.getControls().size() == previousGate.getControls().size()) {
    std::vector<int> controlsCurr =
        getIndicesOfValueRange(currentGate.getControls());
    std::vector<int> controlsPrev =
        getIndicesOfValueRange(previousGate.getControls());
    // sort both arrays
    std::sort(controlsCurr.begin(), controlsCurr.end(), std::greater<int>());
    std::sort(controlsPrev.begin(), controlsPrev.end(), std::greater<int>());
    // compare both arrays
    if (!(std::equal(controlsCurr.begin(), controlsCurr.end(),
                     controlsPrev.begin())))
      return;
  } else
    return;
  // so far, controls are the same, now check the targets
  if (currentGate.getTargets().size() == previousGate.getTargets().size()) {
    std::vector<int> targetsCurr =
        getIndicesOfValueRange(currentGate.getTargets());
    std::vector<int> targetsPrev =
        getIndicesOfValueRange(previousGate.getTargets());
    // sort both arrays
    std::sort(targetsCurr.begin(), targetsCurr.end(), std::greater<int>());
    std::sort(targetsPrev.begin(), targetsPrev.end(), std::greater<int>());
    // compare both arrays
    if (!(std::equal(targetsCurr.begin(), targetsCurr.end(),
                     targetsPrev.begin())))
      return;
  } else
    return;
#ifdef DEBUG
  llvm::outs() << "Current Operation: ";
  currentGate->print(llvm::outs());
  llvm::outs() << "\n";
  llvm::outs() << "Previous Operation: ";
  previousGate->print(llvm::outs());
  llvm::outs() << "\n";
#endif
  // At this point, I should de able to remove the pattern
  mlir::IRRewriter rewriter(currentGate->getContext());
  // Erase the operations
  rewriter.eraseOp(currentGate);
  rewriter.eraseOp(previousGate);
}
} // namespace mqss_cudaq::support::transforms
