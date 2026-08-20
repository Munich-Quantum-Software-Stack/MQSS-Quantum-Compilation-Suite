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

#include "Passes/Transforms/PassUtils.h"
#include "Utils/DebugUtils.h"

namespace mqssci::opt {

#define GEN_PASS_DEF_COMMONREDUCTIONPASS
#include "Passes/Transforms/Transforms.h.inc"

} // namespace mqssci::opt

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { HXHToZ, HZHToX, SAdjZToS, SZToSAdj, NA };

struct SharedPassLogic {

  void run(MyModuleAnalysis &analysis, PassMode Mode,
           QuantumDialect DialectTy) {

    ReductionPassInfoty PassInfo;
    if (Mode == PassMode::HXHToZ) {

      PassInfo.GatesToCancel.push_back(H);
      PassInfo.GatesToCancel.push_back(PauliX);
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.NewGateTy = Gate::PauliZ;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::HZHToX) {
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.GatesToCancel.push_back(PauliZ);
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.NewGateTy = Gate::PauliX;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::SAdjZToS) {
      PassInfo.GatesToCancel.push_back(SAdj);
      PassInfo.GatesToCancel.push_back(PauliZ);
      PassInfo.NewGateTy = Gate::S;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::SZToSAdj) {
      PassInfo.GatesToCancel.push_back(S);
      PassInfo.GatesToCancel.push_back(PauliZ);
      PassInfo.NewGateTy = Gate::SAdj;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    }

    performReduction(analysis, PassInfo, DialectTy);
  }
};

struct CommonReduction
    : public mqssci::opt::impl::CommonReductionPassBase<CommonReduction> {

public:
  // Default constructor (required for pass registry)
  CommonReduction() = default;

  // Forward the options constructor to the base
  CommonReduction(const CommonReductionPassOptions &options)
      : CommonReductionPassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    MQSS_DEBUG("\n[Applying Pass: CommonReduction]\n");

    SharedPassLogic PassLogic;

    if (mode == "HZHToX") {
      PassLogic.run(analysis, PassMode::HZHToX, DialectTy);
    } else if (mode == "HXHToZ")
      PassLogic.run(analysis, PassMode::HXHToZ, DialectTy);
    else if (mode == "SAdjZToS")
      PassLogic.run(analysis, PassMode::SAdjZToS, DialectTy);
    else if (mode == "SZToSAdj")
      PassLogic.run(analysis, PassMode::SZToSAdj, DialectTy);
    else {
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

std::unique_ptr<mlir::Pass> mqssci::opt::CommonReductionPass() {
  return std::make_unique<CommonReduction>();
}

std::unique_ptr<mlir::Pass>
mqssci::opt::CommonReductionPass(const CommonReductionPassOptions &options) {
  return std::make_unique<CommonReduction>(options);
}
