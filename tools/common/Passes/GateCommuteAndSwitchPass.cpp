

#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

enum class PassMode {
    SwitchHX,
    SwitchHY,
    SwitchHZ,
    SwitchZH,
    SwitchXH,
    SwitchYH,
    SwitchXYZH,
    SwitchHXYZ,
    NA
};

struct SharedPassLogic{

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo, PassMode Mode) {

    PassInfoty PassInfo;

    if(Mode == PassMode::SwitchXYZH){
        PassInfo.FirstGateTy.push_back(Gate::PauliX);
        PassInfo.FirstGateTy.push_back(Gate::PauliY);
        PassInfo.FirstGateTy.push_back(Gate::PauliZ);

        PassInfo.SecondGateTy.push_back(Gate::H);
        PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
        PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
        PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
        PassInfo.CompareKey = {"Target", "Target"};
    }
    else if(Mode == PassMode::SwitchHXYZ){
        PassInfo.FirstGateTy.push_back(Gate::H);
        PassInfo.SecondGateTy.push_back(Gate::PauliX);
        PassInfo.SecondGateTy.push_back(Gate::PauliY);
        PassInfo.SecondGateTy.push_back(Gate::PauliZ);

        PassInfo.ReplacementMap[Gate::PauliX] = Gate::PauliZ;
        PassInfo.ReplacementMap[Gate::PauliY] = Gate::PauliY;
        PassInfo.ReplacementMap[Gate::PauliZ] = Gate::PauliX;
        PassInfo.CompareKey = {"Target", "Target"};
    }
    
    for (auto &[kernel, QInfo] : KernelDialectInfo) {
      performCommuteAndSwitch(QInfo, PassInfo);
    }
  }
};

class CommonSwitchHXYZ
    : public PassWrapper<CommonSwitchHXYZ, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonSwitchHXYZ)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonSwitchHXYZPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern - Hadamard followed by "
           "Pauli{X,Y,Z} gate and switches it to Pauli{X,Y,Z} followed by "
           "Hadamard";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonSwitchHXYZ]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::SwitchHXYZ);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};


class CommonSwitchXYZH
    : public PassWrapper<CommonSwitchXYZH, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonSwitchXYZH)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonSwitchXYZHPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for the gate Op pattern - Pauli{X,Y,Z} followed by "
           "Hadamard gate and switches it to Hadamard followed by "
           "Pauli{X,Y,Z}";
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonSwitchXYZH]\n";

    SharedPassLogic PassLogic;
    PassLogic.run(analysis.getKernelDialectInfo(), PassMode::SwitchXYZH);

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};


} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonSwitchXYZHPass() {
  return std::make_unique<CommonSwitchXYZH>();
}
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonSwitchHXYZPass() {
  return std::make_unique<CommonSwitchHXYZ>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonSwitchXYZHPass() {
  return std::make_unique<CommonSwitchXYZH>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonSwitchHXYZPass() {
  return std::make_unique<CommonSwitchHXYZ>();
}
#endif