
#include "include/transforms/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { HXHToZ, HZHToX, SAdjZToS, SZToSAdj, NA };

struct SharedPassLogic {

  void run(DialectAnalysis analysis, PassMode Mode) {

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

    performReduction(analysis, PassInfo);
  }
};

class CommonReduction
    : public mqss_backend::CommonReductionPassBase<CommonReduction> {

public:

// Default constructor (required for pass registry)
  CommonReduction() = default;

  // Forward the options constructor to the base
  CommonReduction(const CommonReductionPassOptions &options) : CommonReductionPassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonReduction]\n");

    SharedPassLogic PassLogic;

    if (mode == "HZHToX") {
      PassLogic.run(analysis, PassMode::HZHToX);
    } else if (mode == "HXHToZ")
      PassLogic.run(analysis, PassMode::HXHToZ);
    else if (mode == "SAdjZToS")
      PassLogic.run(analysis, PassMode::SAdjZToS);
    else if (mode == "SZToSAdj")
      PassLogic.run(analysis, PassMode::SZToSAdj);
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

std::unique_ptr<mlir::Pass> mqss_backend::CommonReductionPass() {
  return std::make_unique<CommonReduction>();
}

std::unique_ptr<mlir::Pass> mqss_backend::createCommonReductionPass(
    const CommonReductionPassOptions &options) {
  return std::make_unique<CommonReduction>(options);
}