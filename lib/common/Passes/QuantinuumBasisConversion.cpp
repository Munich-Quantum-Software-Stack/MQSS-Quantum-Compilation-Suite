

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