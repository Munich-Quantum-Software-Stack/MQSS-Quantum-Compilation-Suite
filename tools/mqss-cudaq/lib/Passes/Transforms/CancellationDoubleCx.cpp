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
  date   January 2025
  version 1.0

Adapted from: https://dl.acm.org/doi/10.5555/1972505

*************************************************************************/

// #include "Passes/BaseMQSSPass.hpp"
#include "SemanticExtractLayer/QuakeExtractor.h"
#include "MQSSCUDAQPasses/Transforms.hpp"
#include "MQSSCUDAQPasses/CancellationOperations.hpp"

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Support/Plugin.h"
#include "mlir/IR/Threading.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
// Include auto-generated pass registration
namespace mqss_cudaq::opt {
#define GEN_PASS_DEF_CANCELLATIONDOUBLECX
#include "MQSSCUDAQPasses/Transforms.h.inc"
} // namespace mqss::opt
using namespace mlir;
using namespace mqss_cudaq::support::transforms;

namespace {

class CancellationDoubleCx
    : public PassWrapper<CancellationDoubleCx, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CancellationDoubleCx)

  llvm::StringRef getArgument() const override {
    return "CancellationDoubleCx";
  }
  llvm::StringRef getDescription() const override {
    return "This pass removes the pattern CNot, CNot if both gates operates on "
           "the same control and targets.";
  }

  void runOnOperation() override {
    auto &analysis = getAnalysis<QuakeAnalysis>();
    auto DialectInfo = analysis.getDialectInfo();
    auto kernels = DialectInfo.QuantumKernels;
    for (auto kernel : kernels) {
      kernel->walk([&](Operation *op) {
        patternCancellation<quake::XOp, quake::XOp>(op, 1, 1, 1, 1);
      });
    }
    
  }
};
} // namespace

std::unique_ptr<Pass> mqss_cudaq::opt::createCancellationDoubleCxPass() {
  return std::make_unique<CancellationDoubleCx>();
}
