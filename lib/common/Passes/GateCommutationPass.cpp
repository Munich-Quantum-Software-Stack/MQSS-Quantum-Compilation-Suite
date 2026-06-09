

#include "include/transforms/PassUtils.h"

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

  void run(MyModuleAnalysis &analysis, PassMode Mode) {

    PassInfoty PassInfo;
    if (Mode == PassMode::CommuteCxRx) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::RX);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteCxX) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliX);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteXCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::CommuteCxZ) {
      PassInfo.FirstGateTy.push_back(Gate::CNOT);
      PassInfo.SecondGateTy.push_back(Gate::PauliZ);
      PassInfo.CompareKey = {QubitRole::Control, QubitRole::Target};
    } else if (Mode == PassMode::CommuteZCx) {
      PassInfo.FirstGateTy.push_back(Gate::PauliZ);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Control};
    } else if (Mode == PassMode::CommuteRxCx) {
      PassInfo.FirstGateTy.push_back(Gate::RX);
      PassInfo.SecondGateTy.push_back(Gate::CNOT);
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    }
    performCommutation(analysis, PassInfo);
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

public:
  // Default constructor (required for pass registry)
  CommonCommute() = default;

  // Forward the options constructor to the base
  CommonCommute(const CommonCommutePassOptions &options) : CommonCommutePassBase() {
    mode = options.mode;
  }

  void runOnOperation() override {

    auto analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonCommute]\n");

    SharedPassLogic PassLogic;
    if (mode == "CX-RX")
      PassLogic.run(analysis, PassMode::CommuteCxRx);
    else if (mode == "RX-CX")
      PassLogic.run(analysis, PassMode::CommuteRxCx);
    else if (mode == "CX-X")
      PassLogic.run(analysis, PassMode::CommuteCxX);
    else if (mode == "X-CX")
      PassLogic.run(analysis, PassMode::CommuteXCx);
    else if (mode == "CX-Z")
      PassLogic.run(analysis, PassMode::CommuteCxZ);
    else if (mode == "Z-CX")
      PassLogic.run(analysis, PassMode::CommuteZCx);
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

std::unique_ptr<mlir::Pass> mqss_backend::createCommonCommutePass(
    const CommonCommutePassOptions &options) {
  return std::make_unique<CommonCommute>(options);
}