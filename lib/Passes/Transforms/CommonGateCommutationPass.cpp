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

#define GEN_PASS_DEF_COMMONCOMMUTEPASS
#include "Passes/Transforms/TransformPasses.h.inc"

} // namespace mqss::mqssci::opt

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode {
  CommuteCxRx,
  CommuteCxX,
  CommuteXCx,
  CommuteCxZ,
  CommuteZCx,
  CommuteRxCx,
  NA
};

struct SharedPassLogic {

  std::tuple<bool, llvm::StringRef> run(MyModuleAnalysis &analysis,
                                        PassMode Mode) {

    PassInfoty PassInfo;
    if (Mode == PassMode::CommuteCxRx) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::RX);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteCxX) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliX);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteXCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteCxZ) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliZ);
      PassInfo.CompareKey = {QubitRole::Control, QubitRole::Target};
    } else if (Mode == PassMode::CommuteZCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliZ);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Control};
    } else if (Mode == PassMode::CommuteRxCx) {
      PassInfo.FirstGateTy.push_back(Gate::RX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    }
    auto [result, kernelName, commutedCandidates] =
        performCommutation(analysis, PassInfo);
    return {result, kernelName};
  }
};

/**
 * @brief Commutes two quantum operations if they match a specific pattern.
 *
 * @details This function searches for a specific pattern where operation T2 is
 * followed by T1 and attempts to commute them to the order T1 followed by T2,
 * under the constraints of control and target qubit counts, intermediate
 * modifications etc.
 **/

struct CommonCommute
    : public mqss::mqssci::opt::impl::CommonCommutePassBase<CommonCommute> {

public:
  // Default constructor (required for pass registry)
  CommonCommute() = default;

  // Forward the options constructor to the base
  CommonCommute(const CommonCommutePassOptions &options)
      : CommonCommutePassBase() {
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

    MQSS_DEBUG("\n[Applying Pass: CommonCommute]\n");

    SharedPassLogic PassLogic;
    llvm::StringRef kernelName = "";
    bool success = false;
    if (mode == "CX-RX")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteCxRx);
    else if (mode == "RX-CX")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteRxCx);
    else if (mode == "CX-X")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteCxX);
    else if (mode == "X-CX")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteXCx);
    else if (mode == "CX-Z")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteCxZ);
    else if (mode == "Z-CX")
      std::tie(success, kernelName) =
          PassLogic.run(analysis, PassMode::CommuteZCx);
    else {
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
      return;
    }

    if (!success) {
      llvm::errs() << "CommonCommutation: Failed to apply Pass on: "
                   << kernelName << "\n";
      signalPassFailure();
      return;
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
      signalPassFailure();
      return;
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::CommonCommutePass() {
  return std::make_unique<CommonCommute>();
}

std::unique_ptr<mlir::Pass>
mqss::mqssci::opt::CommonCommutePass(const CommonCommutePassOptions &options) {
  return std::make_unique<CommonCommute>(options);
}
