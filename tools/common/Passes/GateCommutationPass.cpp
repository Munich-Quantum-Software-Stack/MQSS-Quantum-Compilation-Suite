

#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode {
    CommuteCxRx,
    CommuteCxX,
    CommuteXCx,
    CommuteCxZ,
    CommuteZCx,
    CommuteRxCx,
    NA
};

struct SharedPassLogic{

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo,
           PassMode Mode) {

    PassInfoty PassInfo;
    if (Mode == PassMode::CommuteCxRx) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::RX);
      PassInfo.CompareKey = {"Target", "Target"};
    }
    else if (Mode == PassMode::CommuteCxX) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliX);
      PassInfo.CompareKey = {"Target", "Target"};
    } else if (Mode == PassMode::CommuteXCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {"Target", "Target"};
    } else if (Mode == PassMode::CommuteCxZ) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliZ);
      PassInfo.CompareKey = {"Control", "Target"};
    } else if (Mode == PassMode::CommuteZCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliZ);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {"Target", "Control"};
    }
    else if (Mode == PassMode::CommuteRxCx) {
      PassInfo.FirstGateTy.push_back(Gate::RX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {"Target", "Target"};
    }
    for (auto &[kernel, Info] : KernelDialectInfo) {
      performCommutation(Info, PassInfo);
    }
  }
};

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
    return "This pass searches for the gate Op pattern CNOT followed by Rx and "
           "commutes them.";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxRx]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxRx);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonCommuteCxX
    : public PassWrapper<CommonCommuteCxX, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteCxX)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonCommuteCxXPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern CNOT followed by PAULIX "
           "and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxX]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxX);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonCommuteXCx
    : public PassWrapper<CommonCommuteXCx, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteXCx)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonCommuteXCxPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern PAULIX followed by CNOT "
           "and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteXCx]\n";
    
    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteXCx);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonCommuteCxZ
    : public PassWrapper<CommonCommuteCxZ, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteCxZ)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonCommuteCxZPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern CNOT followed by PAULIZ "
           "and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteCxZ]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxZ);
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonCommuteZCx
    : public PassWrapper<CommonCommuteZCx, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCommuteZCx)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonCommuteZCxPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern PAULIZ followed by CNOT "
           "and commutes them.";
  }

  void runOnOperation() override {

#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteZCx]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteZCx);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
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
    return "This pass searches for the gate Op pattern Rx followed by CNOT and "
           "commutes them.";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonCommuteRxCx]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteRxCx);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
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
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteXCxPass() {
  return std::make_unique<CommonCommuteXCx>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteCxZPass() {
  return std::make_unique<CommonCommuteCxZ>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteZCxPass() {
  return std::make_unique<CommonCommuteZCx>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCommuteRxCxPass() {
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
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteXCxPass() {
  return std::make_unique<CommonCommuteXCx>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteCxZPass() {
  return std::make_unique<CommonCommuteCxZ>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteZCxPass() {
  return std::make_unique<CommonCommuteZCx>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCommuteRxCxPass() {
  return std::make_unique<CommonCommuteRxCx>();
}

#endif