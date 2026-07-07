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
  author Akshay Bhosale
  date   February 2026
  version 1.0
*************************************************************************/

#include "Passes/transforms/PassUtils.h"

namespace mqss::opt {
  
#define GEN_PASS_DEF_COMMONGATECANCELLATIONPASS
#include "Passes/transforms/Transforms.h.inc"

} // namespace mqss::opt


using namespace mlir;
using namespace llvm;

namespace {

class CommonGateCancellation
    : public mqss_backend::impl::CommonGateCancellationPassBase<CommonGateCancellation> {

  public:
  // Default constructor (required for pass registry)
  CommonGateCancellation() = default;

  // Forward the options constructor to the base
   CommonGateCancellation(const CommonGateCancellationPassOptions &options) : CommonGateCancellationPassBase() {
    mode = options.mode;
  }

  std::vector<Gate> GatesToCancel{CNOT, PauliX, PauliZ, PauliY, H, Hadamard};

  std::vector<Gate> RotationGatesToCancel{RX, RY, RZ};
  // The following algorithm, iterates over operations in an
  // mlir-kernel (FuncOp) attempting to erase consecutive CNOTs under
  // certain conditions. The conditions are:
  // 1. The two CNOTs should appear consecutively
  // 2. There should not be any intervening operation between the
  // CNOTs that operating on
  //    the Input/Output Qubits of the CNOTs.
  // 3. The two CNOTs should operate on the same Input Qubits
  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.

    
    MQSS_DEBUG("[Applying Pass: CommonGateCancellationPass]\n");

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto KernelDialectInfo = analysis.getKernelDialectInfo();

    // Empty CompareKey meaning - both control and target qubit operands
    // of the gates to be cancelled will be compared


    int count = 0;
    if (mode == "CancelGate") {
      Comparety CompareKey;

      for (auto &[kernel, Info] : KernelDialectInfo) {
        MQSS_DEBUG(++count << ". kernel: " << kernel.getSymName() << "\n");
        performCancellation(Info.OpQViewMap, GatesToCancel, CompareKey);
        MQSS_DEBUG( "\n");
      }
    } else if (mode == "CancelNullRotation") {

      for (auto &[kernel, Info] : KernelDialectInfo) {
        MQSS_DEBUG(++count << ". kernel: " << kernel.getSymName() << "\n");
        performNullRotationCancellation(Info.OpQViewMap, RotationGatesToCancel);
        MQSS_DEBUG( "\n");
      }
    } else {
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonGateCancellationPass() {
  return std::make_unique<CommonGateCancellation>();
}

std::unique_ptr<mlir::Pass> mqss_backend::createCommonGateCancellationPass(
    const CommonGateCancellationPassOptions &options) {
  return std::make_unique<CommonGateCancellation>(options);
}