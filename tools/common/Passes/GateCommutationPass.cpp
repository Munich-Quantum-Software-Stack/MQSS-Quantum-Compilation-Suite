

#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "include/utils.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonCommuteCxRx
    : public PassWrapper<CommonCommuteCxRx, OperationPass<mlir::ModuleOp>> {

    public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteCxRx)

  [[nodiscard]] StringRef getArgument() const override {
      return "CommonCommuteCxRxPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern CNOT followed by Rx and commutes them.";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxRx]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::CNOT;
    auto SecondGateTy = Gate::RX;

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy);
    }
  }
};

class CommonCommuteCxX : public PassWrapper<CommonCommuteCxX, OperationPass<mlir::ModuleOp>> {

  public:
  public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteCxX)

  [[nodiscard]] StringRef getArgument() const override {
      return "CommonCommuteCxXPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern CNOT followed by PAULIX and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxX]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::CNOT;
    auto SecondGateTy = Gate::PAULIX;

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy);
    }
  }
};
} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteCxRxPass() {
  return std::make_unique<CommonCommuteCxRx>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteCxXPass() {
  return std::make_unique<CommonCommuteCxX>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxRxPass() {
  return std::make_unique<CommonCommuteCxRx>();
}

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxXPass() {
  return std::make_unique<CommonCommuteCxX>();
}
#endif