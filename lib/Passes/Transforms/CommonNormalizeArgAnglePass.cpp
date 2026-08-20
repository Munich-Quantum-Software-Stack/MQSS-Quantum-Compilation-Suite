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

namespace mqss::mqssci::opt {

#define GEN_PASS_DEF_COMMONNORMALIZEARGANGLEPASS
#include "Passes/Transforms/Transforms.h.inc"

} // namespace mqss::mqssci::opt

using namespace mlir;
using namespace llvm;

namespace {

struct CommonNormalizeArgAngle
    : public mqss::mqssci::opt::impl::CommonNormalizeArgAnglePassBase<
          CommonNormalizeArgAngle> {

public:
  void runOnOperation() override {

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    MQSS_DEBUG("\n[Applying Pass: NormalizeAngle]\n");

    for (auto [kernel, Info] : analysis.getKernelDialectInfo())
      performArgAngelNormalization(Info.OpQViewMap, DialectTy);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::CommonNormalizeArgAnglePass() {
  return std::make_unique<CommonNormalizeArgAngle>();
}
