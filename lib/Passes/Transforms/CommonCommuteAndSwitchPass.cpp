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

#include "Passes/Transforms/PassLogic.h"
#include "Passes/Transforms/TransformPasses.h"
#include "Utils/DebugUtils.h"

namespace mqss::mqssci::opt {

#define GEN_PASS_DEF_COMMONSWITCHPASS
#include "Passes/Transforms/TransformPasses.h.inc"

} // namespace mqss::mqssci::opt

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { SwitchXYZH, SwitchHXYZ };

struct SharedPassLogic {

  tuple<bool, llvm::StringRef> run(MyModuleAnalysis &analysis, PassMode Mode,
                                   QuantumDialect DialectTy) {

    PassInfoty PassInfo;

    if (Mode == PassMode::SwitchXYZH) {
      PassInfo.FirstGateTy.push_back(Gate::PauliX);
      PassInfo.FirstGateTy.push_back(Gate::PauliY);
      PassInfo.FirstGateTy.push_back(Gate::PauliZ);

      PassInfo.SecondGateTy.push_back(Gate::H);
      PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
      PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
      PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::SwitchHXYZ) {
      PassInfo.FirstGateTy.push_back(Gate::H);
      PassInfo.SecondGateTy.push_back(Gate::PauliX);
      PassInfo.SecondGateTy.push_back(Gate::PauliY);
      PassInfo.SecondGateTy.push_back(Gate::PauliZ);

      PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
      PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
      PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    }

    return performCommuteAndSwitch(analysis, PassInfo, DialectTy);
  }
};

struct CommonSwitch
    : public mqss::mqssci::opt::impl::CommonSwitchPassBase<CommonSwitch> {

public:
  // Default constructor (required for pass registry)
  CommonSwitch() = default;

  // Forward the options constructor to the base
  CommonSwitch(const CommonSwitchPassOptions &options)
      : CommonSwitchPassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {
    auto &selector = getAnalysis<DialectAnalysisSelector>();
    if (!selector.get()) {

      llvm::errs() << "[" << getArgument() << "]"
                   << " : Invalid MLIR Dialect\n";
      signalPassFailure();
      return;
    }
    auto &analysis = *selector.get();
    if (!analysis.isValid()) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : Invalid Module Analysis\n";
      signalPassFailure();
      return;
    }
    auto DialectTy = selector.getDialect();

    MQSS_DEBUG("\n[Applying Pass: CommonGateSwitch]\n");

    SharedPassLogic PassLogic;
    bool success = false;
    llvm::StringRef kernelName = "";
    if (mode == "XYZHtoHXYZ")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::SwitchXYZH, DialectTy);
    else if (mode == "HXYZtoXYZH")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::SwitchHXYZ, DialectTy);
    else {
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
      return;
    }

    if (!success) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : Failed to apply Pass on: " << kernelName << "\n";
      signalPassFailure();
      return;
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
      signalPassFailure();
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::CommonSwitchPass() {
  return std::make_unique<CommonSwitch>();
}

std::unique_ptr<mlir::Pass>
mqss::mqssci::opt::CommonSwitchPass(const CommonSwitchPassOptions &options) {
  return std::make_unique<CommonSwitch>(options);
}
