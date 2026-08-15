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
#include "Passes/Transforms/Decomposition.h"
#include "Utils/debug_utils.h"

#include <cmath>

namespace mqss::opt {

#define GEN_PASS_DEF_COMMONDECOMPOSITIONPASS
#include "Passes/Transforms/Transforms.h.inc"

} // namespace mqss::opt

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { CxToHCzH, CzToHCxH, HToRzXRz, CHToCX, NA };

struct SharedPassLogic {

  void run(MyModuleAnalysis &analysis, PassMode passmode,
           QuantumDialect DialectTy) {

    DecomposePassInfoTy PassInfo;
    if (passmode == PassMode::CxToHCzH) {
      PassInfo.GateToDecompose = Gate::CNOT;
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::CZ, {}});
      PassInfo.Pattern.push_back({Gate::H, {}});
    } else if (passmode == PassMode::CzToHCxH) {
      PassInfo.GateToDecompose = Gate::CZ;
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::CNOT, {}});
      PassInfo.Pattern.push_back({Gate::H, {}});
    } else if (passmode == PassMode::HToRzXRz) {

      PassInfo.GateToDecompose = Gate::H;
      PassInfo.Pattern.push_back({Gate::RZ, {M_PI}});
      PassInfo.Pattern.push_back({Gate::PauliX, {}});
      PassInfo.Pattern.push_back({Gate::RZ, {M_PI_2}});
    } else if (passmode == PassMode::CHToCX) {
      // quake.h control, target
      // ───────────────────────────────────
      // quake.s target;
      // quake.h target;
      // quake.t target;
      // quake.x control, target;
      // quake.t<adj> target;
      // quake.h target;
      // quake.s<adj> target;
      PassInfo.GateToDecompose = Gate::CH;
      PassInfo.Pattern.push_back({Gate::S, {}});
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::T, {}});
      PassInfo.Pattern.push_back({Gate::CNOT, {}});
      PassInfo.Pattern.push_back({Gate::TAdj, {}});
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::SAdj, {}});
    }

    performDecomposition(analysis, PassInfo, DialectTy);
  }
};

class CommonDecomposition
    : public mqss_backend::impl::CommonDecompositionPassBase<
          CommonDecomposition> {

public:
  // Default constructor (required for pass registry)
  CommonDecomposition() = default;

  // Forward the options constructor to the base
  CommonDecomposition(const CommonDecompositionPassOptions &options)
      : CommonDecompositionPassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    MQSS_DEBUG("\n[Applying Pass: CommonDecomposition]\n");

    SharedPassLogic PassLogic;

    auto passmode = getPassMode(mode);

    if (passmode != PassMode::NA) {
      PassLogic.run(analysis, passmode, DialectTy);
    } else {
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }

private:
  PassMode getPassMode(const StringRef &mode) {
    if (mode == "CxToHCzH") {
      return PassMode::CxToHCzH;
    } else if (mode == "CzToHCxH") {
      return PassMode::CzToHCxH;
    } else if (mode == "HToRzXRz") {
      return PassMode::HToRzXRz;
    } else if (mode == "CHToCX") {
      return PassMode::CHToCX;
    }
    return PassMode::NA;
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonDecompositionPass() {
  return std::make_unique<CommonDecomposition>();
}

std::unique_ptr<mlir::Pass> mqss_backend::createCommonDecompositionPass(
    const CommonDecompositionPassOptions &options) {
  return std::make_unique<CommonDecomposition>(options);
}
