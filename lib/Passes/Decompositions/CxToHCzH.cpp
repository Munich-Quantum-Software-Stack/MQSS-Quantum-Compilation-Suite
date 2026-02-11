/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

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
*************************************************************************
  author Martin Letras
  date   December 2024
  version 1.0

Adapted from: https://quantumcomputing.stackexchange.com/a/13784

*************************************************************************/

#include "Passes/BaseMQSSPass.hpp"
#include "Passes/Decompositions.hpp"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Support/Plugin.h"
#include "mlir/IR/Threading.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

// Include auto-generated pass registration
namespace mqss::opt {
#define GEN_PASS_DEF_CXTOHCZH
#include "Passes/Decompositions.h.inc"
} // namespace mqss::opt
using namespace mlir;

namespace {

void CxToHCzHPattern(mlir::Operation *currentOp) {
  auto cxOp = dyn_cast_or_null<quake::XOp>(*currentOp);
  if (!cxOp)
    return;
  if (cxOp.getControls().size() != 1 || cxOp.getTargets().size() != 1)
    return; // if the cx operation is not two qubit then do nothing
  // Get the operands of the XOp (control and target qubits)
  Value control = cxOp.getControls()[0];
  Value target = cxOp.getTargets()[0];
  Location loc = cxOp.getLoc();
  // TODO: howt to pass hOp.isAdj(), hOp.getParameters()
  // Insert the H gate on the original target qubit
  mlir::IRRewriter rewriter(cxOp->getContext());
  rewriter.setInsertionPointAfter(cxOp);
  rewriter.create<quake::HOp>(loc, target);
  // Insert the Z gate swapping the control and target qubits
  rewriter.create<quake::ZOp>(loc, target, control);
  // Insert the H gate on the original target qubit
  rewriter.create<quake::HOp>(loc, target);
  // Erase the original Cx operation
  rewriter.eraseOp(cxOp);
}

class CxToHCzH : public BaseMQSSPass<CxToHCzH> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CxToHCzH)

  llvm::StringRef getArgument() const override { return "CxToHCzH"; }
  llvm::StringRef getDescription() const override {
    return "Decomposition pass of two-qubits cnot by H, Cz, and H";
  }

  void operationsOnQuantumKernel(func::FuncOp kernel) override {
    kernel.walk([&](Operation *op) { CxToHCzHPattern(op); });
  }
};
} // namespace

std::unique_ptr<mlir::Pass> mqss::opt::createCxToHCzHDecompositionPass() {
  return std::make_unique<CxToHCzH>();
}
