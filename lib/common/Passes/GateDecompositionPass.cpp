

#include "include/transforms/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { CxToHCzH, CzToHCxH, ReverseCx, NA };

struct SharedPassLogic {

  void run(DialectAnalysis analysis, PassMode passmode) {

    if (passmode == PassMode::CxToHCzH || passmode == PassMode::CzToHCxH) {
        performDecomposition(analysis);
    }
    if (passmode == PassMode::ReverseCx) {
        performDecomposition(analysis, true);
    }
  }
};

class CommonDecomposition : public mqss_backend::CommonDecompositionPassBase<class CommonDecomposition> {

public:
  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonDecomposition]\n");

    SharedPassLogic PassLogic;

    if (mode == "CxToHCzH") {
      PassLogic.run(analysis, PassMode::CxToHCzH);
    } else if (mode == "CzToHCxH") {
      PassLogic.run(analysis, PassMode::CzToHCxH);
    } else if (mode == "ReverseCx") {
      PassLogic.run(analysis, PassMode::ReverseCx);
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

std::unique_ptr<mlir::Pass> mqss_backend::CommonDecompositionPass() {
  return std::make_unique<CommonDecomposition>();
}