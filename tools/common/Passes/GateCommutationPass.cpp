

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

struct SharedPassLogic {

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo,
           PassMode Mode) {

    PassInfoty PassInfo;
    if (Mode == PassMode::CommuteCxRx) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::RX);
      PassInfo.CompareKey = {"Target", "Target"};
    } else if (Mode == PassMode::CommuteCxX) {
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
    } else if (Mode == PassMode::CommuteRxCx) {
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

class CommonCommute
    : public mqss_backend::CommonCommutePassBase<CommonCommute> {

  using Base = mqss_backend::CommonCommutePassBase<CommonCommute>;
  using Base::Base;

  void runOnOperation() override {

    auto analysis = getAnalysis<DialectAnalysis>();

    llvm::outs() << "\n[Applying Pass: CommonCommute]\n";

    SharedPassLogic PassLogic;
    if (mode == "CX-RX")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxRx);
    else if (mode == "RX-CX")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteRxCx);
    else if (mode == "CX-X")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxX);
    else if (mode == "X-CX")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteXCx);
    else if (mode == "CX-Z")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteCxZ);
    else if (mode == "Z-CX")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::CommuteZCx);
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

std::unique_ptr<mlir::Pass> mqss_backend::CommonCommutePass() {
  return std::make_unique<CommonCommute>();
}
