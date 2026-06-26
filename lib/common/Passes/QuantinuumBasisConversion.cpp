
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

// #include "include/transforms/Decomposition.h"
#include "include/transforms/DecompositionPatternSelection.h"

#include <cmath>
#include <memory>
#include <utility>

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { CxToHCzH, CzToHCxH, HToRzXRz, CHToCX, NA };


struct CxToHCzH : public DecompositionPattern {
  CxToHCzH() {
    name = "CxToHCzH";
    sourceOp = OperatorInfo("CNOT");
    targetOps = {OperatorInfo("H"), OperatorInfo("CZ"), OperatorInfo("H")};
  }

  // "rewrite" = given the source gate metadata, produce the target gates
  void apply(MyModuleAnalysis &analysis) const override {

    DecomposePassInfoTy PassInfo;
    PassInfo.GateToDecompose = Gate::CNOT;
    PassInfo.Pattern.push_back({Gate::H, {}});
    PassInfo.Pattern.push_back({Gate::CZ, {}});
    PassInfo.Pattern.push_back({Gate::H, {}});

    performDecomposition(analysis, PassInfo);
  }
};

struct CzToHCxH : public DecompositionPattern {
  CzToHCxH() {
    name = "CzToHCxH";
    sourceOp = OperatorInfo("CZ");
    targetOps = {OperatorInfo("H"), OperatorInfo("CNOT"), OperatorInfo("H")};
  }

  // "rewrite" = given the source gate metadata, produce the target gates
  void apply(MyModuleAnalysis &analysis) const override {

    DecomposePassInfoTy PassInfo;
    PassInfo.GateToDecompose = Gate::CZ;
    PassInfo.Pattern.push_back({Gate::H, {}});
    PassInfo.Pattern.push_back({Gate::CNOT, {}});
    PassInfo.Pattern.push_back({Gate::H, {}});

    performDecomposition(analysis, PassInfo);
  }
};

class QuantBasisConversion : public mqss_backend::QuantBasisConversionPassBase<
                                 class QuantBasisConversion> {

public:
  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: QuantBasisConversion]\n");

    std::vector<std::unique_ptr<DecompositionPattern>> registry;
    registry.push_back(std::make_unique<CxToHCzH>());

    // 2. Build the graph and run Dijkstra against target basis
    DecompositionGraph graph{std::move(registry), analysis};

    // Convert targetBasis, disabledPatterns and enabledPatterns to sets for
    // O(1) lookup 

    // Quantinuum Basis
    std::vector<std::string> targetBasis = {"H",  "S", "T", "RX", "RY",
                                            "RZ", "PauliX", "PauliY", "PauliZ",  "CNOT"};

    SmallVector<OperatorInfo, 8> legalOperatorSet;
    for (const std::string &targetInfo : targetBasis)
      legalOperatorSet.emplace_back(targetInfo);

    std::unordered_set<OperatorInfo> basisGatesSet(legalOperatorSet.begin(),
                                                   legalOperatorSet.end());

    static SmallVector<std::string> disabledPatterns = {"R1ToU3"};

    std::unordered_set<std::string> disabledPatternsSet(
        disabledPatterns.begin(), disabledPatterns.end());

    // // 3. Ask the graph which patterns are on shortest paths to the basis
    // //    → returns e.g. {"HToPhasedRx", "SToPhasedRx", "TToPhasedRx", ...}
    graph.selectAndApplyPatterns(basisGatesSet, disabledPatternsSet);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::QuantBasisConversionPass() {
  return std::make_unique<QuantBasisConversion>();
}