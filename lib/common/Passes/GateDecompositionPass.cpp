

#include "include/transforms/Decomposition.h"
#include <cmath>

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode { CxToHCzH, CzToHCxH, HToRzXRz, NA };

struct SharedPassLogic {

  void run(DialectAnalysis analysis, PassMode passmode) {

    DecomposePassInfoTy PassInfo;
    if (passmode == PassMode::CxToHCzH) {
      PassInfo.GateToDecompose = Gate::CNOT;
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::CZ, {}});
      PassInfo.Pattern.push_back({Gate::H, {}});
    } else if (passmode == PassMode::CzToHCxH) {
      PassInfo.GateToDecompose = Gate::CZ;
      PassInfo.Pattern.push_back({Gate::H, {}});
      PassInfo.Pattern.push_back({Gate::CNOT,{}});
      PassInfo.Pattern.push_back({Gate::H, {}});
    }
    else if (passmode == PassMode::HToRzXRz) {

      PassInfo.GateToDecompose = Gate::H;    
      PassInfo.Pattern.push_back({Gate::RZ, {M_PI}});
      PassInfo.Pattern.push_back({Gate::PauliX, {}});
      PassInfo.Pattern.push_back({Gate::RZ, {M_PI_2}});
    }
    performDecomposition(analysis, PassInfo);
  }
};

class CommonDecomposition : public mqss_backend::CommonDecompositionPassBase<class CommonDecomposition> {

public:
  void runOnOperation() override {

    auto &analysis = getAnalysis<DialectAnalysis>();

    MQSS_DEBUG("\n[Applying Pass: CommonDecomposition]\n");

    SharedPassLogic PassLogic;

    auto passmode = getPassMode(mode);
    if(passmode != PassMode::NA){
      PassLogic.run(analysis, passmode);
    } 
    else {
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }

  private:
    PassMode getPassMode(const StringRef &mode) {
    if (mode == "CxToHCzH") {
      return PassMode::CxToHCzH;
    } else if (mode == "CzToHCxH") {
      return PassMode::CzToHCxH;
    } else if (mode == "HToRzXRz") {
      return PassMode::HToRzXRz;
    }
      return PassMode::NA;
    }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonDecompositionPass() {
  return std::make_unique<CommonDecomposition>();
}