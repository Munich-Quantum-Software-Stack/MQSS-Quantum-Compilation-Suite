
#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonNormalizeArgAngle
    : public mqss_backend::CommonNormalizeArgAnglePassBase<
          CommonNormalizeArgAngle> {

public:
  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    llvm::outs() << "\n[Applying Pass: NormalizeAngle]\n";

    for (auto [kernel, Info] : analysis.getKernelDialectInfo())
      performArgAngelNormalization(Info);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonNormalizeArgAnglePass() {
  return std::make_unique<CommonNormalizeArgAngle>();
}