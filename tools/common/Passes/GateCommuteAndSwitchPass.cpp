

#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonSwitchHX
    : public PassWrapper<CommonSwitchHX, OperationPass<mlir::ModuleOp>> {

    public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonSwitchHX)

  [[nodiscard]] StringRef getArgument() const override {
      return "CommonSwitchHX";
  }
  [[nodiscard]] StringRef getDescription() const override {
      return "This pass searches for the gate Op pattern Hadamard followed by "
             "X and switched it to Z followed by "
             "Hadamard";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonSwitchHX]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::H;
    auto SecondGateTy = Gate::PauliX;
    auto ReplaceGateTy = Gate::PauliZ;

    Comparety CompareKey{"Target", "Target"};
    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommuteAndSwitch(curr_op, OpQuantumView, FirstGateTy, SecondGateTy, ReplaceGateTy, CompareKey);
    }
  }
};

} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonSwitchHXPass() {
  return std::make_unique<CommonSwitchHX>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonSwitchHXPass() {
  return std::make_unique<CommonSwitchHX>();
}
#endif