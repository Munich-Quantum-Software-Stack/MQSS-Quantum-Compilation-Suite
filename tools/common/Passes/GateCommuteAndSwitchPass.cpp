

#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode {
    SwitchHX,
    SwitchHZ,
    NA
};

struct SharedPassLogic{

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo, PassMode Mode) {

    PassInfoty PassInfo;
    if(Mode == PassMode::SwitchHX){   
        PassInfo.FirstGateTy = Gate::H;
        PassInfo.SecondGateTy = Gate::PauliX;
        PassInfo.ReplaceGateTy = Gate::PauliZ;
        PassInfo.CompareKey = {"Target", "Target"};
    }
    else if(Mode == PassMode::SwitchHZ){
        PassInfo.FirstGateTy = Gate::H;
        PassInfo.SecondGateTy = Gate::PauliZ;
        PassInfo.ReplaceGateTy = Gate::PauliX;
        PassInfo.CompareKey = {"Target", "Target"};
    }
    
    for (auto &[kernel, QInfo] : KernelDialectInfo) {
      performCommuteAndSwitch(QInfo, PassInfo);
    }
  }
};

class CommonSwitchHX
    : public PassWrapper<CommonSwitchHX, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonSwitchHX)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonSwitchHXPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern - Hadamard followed by "
           "PauliX and switches it to PauliZ followed by "
           "Hadamard";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonSwitchHX]\n";    

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::SwitchHX);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonSwitchHZ
    : public PassWrapper<CommonSwitchHZ, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonSwitchHZ)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonSwitchHZPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern - Hadamard followed by "
           "PauliZ and switches it to PauliX followed by "
           "Hadamard";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonSwitchHZ]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::SwitchHZ);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonSwitchHXPass() {
  return std::make_unique<CommonSwitchHX>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonSwitchHZPass() {
  return std::make_unique<CommonSwitchHZ>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonSwitchHXPass() {
  return std::make_unique<CommonSwitchHX>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonSwitchHZPass() {
  return std::make_unique<CommonSwitchHZ>();
}
#endif