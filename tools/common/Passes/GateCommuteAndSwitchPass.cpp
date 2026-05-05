

#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { SwitchXYZH, SwitchHXYZ, NA };

struct SharedPassLogic {

  void run(MyModuleAnalysis &analysis, PassMode Mode) {

    PassInfoty PassInfo;

    if (Mode == PassMode::SwitchXYZH) {
      PassInfo.FirstGateTy.push_back(Gate::PauliX);
      PassInfo.FirstGateTy.push_back(Gate::PauliY);
      PassInfo.FirstGateTy.push_back(Gate::PauliZ);

      PassInfo.SecondGateTy.push_back(Gate::H);
      PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
      PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
      PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    } else if (Mode == PassMode::SwitchHXYZ) {
      PassInfo.FirstGateTy.push_back(Gate::H);
      PassInfo.SecondGateTy.push_back(Gate::PauliX);
      PassInfo.SecondGateTy.push_back(Gate::PauliY);
      PassInfo.SecondGateTy.push_back(Gate::PauliZ);

      PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
      PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
      PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
      PassInfo.CompareKey = {QubitRole::Target, QubitRole::Target};
    }

    performCommuteAndSwitch(analysis, PassInfo);
  }
};

class CommonSwitch : public mqss_backend::CommonSwitchPassBase<CommonSwitch> {

public:
  void runOnOperation() override {
    auto &analysis = getAnalysis<DialectAnalysis>();

    llvm::outs() << "\n[Applying Pass: CommonGateSwitch]\n";

    SharedPassLogic PassLogic;
    if (mode == "XYZHtoHXYZ")
      PassLogic.run(analysis, PassMode::SwitchXYZH);
    else if (mode == "HXYZtoXYZH")
      PassLogic.run(analysis, PassMode::SwitchHXYZ);
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

std::unique_ptr<mlir::Pass> mqss_backend::CommonSwitchPass() {
  return std::make_unique<CommonSwitch>();
}