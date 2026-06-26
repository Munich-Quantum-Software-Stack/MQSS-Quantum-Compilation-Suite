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

#include "include/transforms/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { SwitchXYZH, SwitchHXYZ, NA };

struct SharedPassLogic {

  void run(MyModuleAnalysis &analysis, PassMode Mode) {

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

    performCommuteAndSwitch(analysis, PassInfo);
  }
};

class CommonSwitch : public mqss_backend::CommonSwitchPassBase<CommonSwitch> {

public:
  // Default constructor (required for pass registry)
  CommonSwitch() = default;

  // Forward the options constructor to the base
  CommonSwitch(const CommonSwitchPassOptions &options) : CommonSwitchPassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {
    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonGateSwitch]\n");

    SharedPassLogic PassLogic;
    if (mode == "XYZHtoHXYZ")
      PassLogic.run(analysis, PassMode::SwitchXYZH);
    else if (mode == "HXYZtoXYZH")
      PassLogic.run(analysis, PassMode::SwitchHXYZ);
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

std::unique_ptr<mlir::Pass> mqss_backend::CommonSwitchPass() {
  return std::make_unique<CommonSwitch>();
}

std::unique_ptr<mlir::Pass> mqss_backend::createCommonSwitchPass(
    const CommonSwitchPassOptions &options) {
  return std::make_unique<CommonSwitch>(options);
}