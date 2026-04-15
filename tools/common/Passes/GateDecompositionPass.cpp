

#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { CxToHCzH, CzToHCxH, ReverseCx, NA };

struct SharedPassLogic {

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo,
           PassMode passmode) {

    if (passmode == PassMode::CxToHCzH || passmode == PassMode::CzToHCxH) {
      for (auto &[kernel, QInfo] : KernelDialectInfo) {
        performDecomposition(QInfo);
      }
    }
    if (passmode == PassMode::ReverseCx) {
      for (auto &[kernel, QInfo] : KernelDialectInfo) {
        performDecomposition(QInfo, true);
      }
    }
  }
};

class CommonDecomposition : public mqss_backend::CommonDecompositionPassBase<
                                class CommonDecomposition> {

public:
  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    llvm::outs() << "\n[Applying Pass: CommonDecomposition]\n";

    SharedPassLogic PassLogic;

    if (mode == "CxToHCzH") {
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CxToHCzH);
    } else if (mode == "CzToHCxH") {
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CzToHCxH);
    } else if (mode == "ReverseCx") {
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::ReverseCx);
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