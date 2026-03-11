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

Adapted from:
https://quantumcomputing.stackexchange.com/questions/12458/show-that-a-cz-gate-can-be-implemented-using-a-cnot-gate-and-hadamard-gates

*************************************************************************/

#include "MQSSCUDAQPasses/Transforms.hpp"
#include "MQSSCUDAQPasses/CommutateOperations.hpp"
// #include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
// #include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Support/Plugin.h"
#include "mlir/IR/Threading.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "SemanticExtractLayer/QuakeExtractor.h"

// Include auto-generated pass registration
namespace mqss_cudaq::opt {
#define GEN_PASS_DEF_COMMUTECXRX
#include "MQSSCUDAQPasses/Transforms.h.inc"
} // namespace cudaq::opt
using namespace mlir;
using namespace mqss_cudaq::support::transforms;

namespace {

class CommuteCxRx : public PassWrapper<CommuteCxRx, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommuteCxRx)

  llvm::StringRef getArgument() const override { return "CommuteCxRx"; }
  llvm::StringRef getDescription() const override {
    return "Apply commutation pass of pattern CNot-Rx";
  }

  // void operationsOnQuantumKernel(func::FuncOp kernel) override {
  //   // kernel.walk([&](Operation *op) {
  //   //   commuteOperation<quake::XOp, quake::RxOp>(op, 1, 1, 0, 1);
  //   //   // CommuteCNotRx(op);
  //   // });
  // }
  void runOnOperation() override{
      llvm::outs() << "Within the pass: CommuteCxRx\n";
      auto &analysis = getAnalysis<QuakeAnalysis>();
      auto DialectInfo = analysis.getDialectInfo();
      auto kernels = DialectInfo.QuantumKernels;
      for(auto kernel : kernels){
           kernel->walk([&](Operation *op) {
            commuteOperation<quake::XOp, quake::RxOp>(op, 1, 1, 0, 1);
      // CommuteCNotRx(op);
    });

      }
  }
};
} // namespace

std::unique_ptr<Pass> mqss_cudaq::opt::createCommuteCxRxPass() {
  return std::make_unique<CommuteCxRx>();
}
