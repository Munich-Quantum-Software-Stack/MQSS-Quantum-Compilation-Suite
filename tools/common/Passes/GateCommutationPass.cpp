

#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

/**
 * @brief Commutes two quantum operations if they match a specific pattern.
 *
 * @details This function searches for a specific pattern where operation T2 is
 * followed by T1 and attempts to commute them to the order T1 followed by T2,
 * under the constraints of control and target qubit counts, intermediate
 * modifications etc.
 **/

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

    Comparety CompareKey{"Target", "Target"};
    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy,
                         CompareKey);
    }
  }
};

class CommonCommuteCxX : public PassWrapper<CommonCommuteCxX, OperationPass<mlir::ModuleOp>> {

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
    auto SecondGateTy = Gate::PauliX;

    Comparety CompareKey{"Target", "Target"};
    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy, CompareKey);
    }
  }
};

class CommonCommuteCxZ : public PassWrapper<CommonCommuteCxZ, OperationPass<mlir::ModuleOp>> {

  public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteCxZ)

  [[nodiscard]] StringRef getArgument() const override {
      return "CommonCommuteCxZPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern CNOT followed by PAULIZ and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxZ]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::CNOT;
    auto SecondGateTy = Gate::PauliZ;

    // Compare the Control Qubit of Gate1 with Target Qubit of Gate2.
    Comparety CompareKeys{"Control", "Target"};

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy, CompareKeys);
    }
  }
};

class CommonCommuteRxCx
    : public PassWrapper<CommonCommuteRxCx, OperationPass<mlir::ModuleOp>> {

    public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteRxCx)

  [[nodiscard]] StringRef getArgument() const override {
      return "CommonCommuteRxCxPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern Rx followed by CNOT and commutes them.";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonCommuteRxCx]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::RX;
    auto SecondGateTy = Gate::CNOT;

    Comparety CompareKey{"Target", "Target"};
    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy,
                         CompareKey);
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
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteCxZPass(){
  return std::make_unique<CommonCommuteCxZ>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteRxCxPass(){
  return std::make_unique<CommonCommuteRxCx>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxRxPass() {
  return std::make_unique<CommonCommuteCxRx>();
}

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxXPass() {
  return std::make_unique<CommonCommuteCxX>();
}

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxZPass() {
  return std::make_unique<CommonCommuteCxZ>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteRxCxPass() {
  return std::make_unique<CommonCommuteRxCx>();
}

#endif