

#include "include/transforms/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {


class CommonCNOTReverse : public mqss_backend::CommonCNOTReversePassBase<class CommonCNOTReverse> {

public:

  // Default constructor (required for pass registry)
  CommonCNOTReverse() = default;

  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonDecomposition]\n");

    performCNOTReversal(analysis);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonCNOTReversePass() {
  return std::make_unique<CommonCNOTReverse>();
}