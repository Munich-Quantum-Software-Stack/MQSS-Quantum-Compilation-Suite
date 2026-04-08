


#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/Support/Casting.h"

#include "llvm/Support/raw_ostream.h"
#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonGateCancellation
    : public PassWrapper<CommonGateCancellation, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonGateCancellation)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonGateCancellationPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass removes the pattern Gate, Gate if both gates operates on "
           "the same control and targets.";
  }

  std::vector<Gate> GatesToCancel{CNOT, PauliX, PauliZ, PauliY, H, Hadamard};

  std::vector<Gate> RotationGatesToCancel{RX, RZ, RZ};
  // The following algorithm, iterates over operations in an mlir-kernel
  // (FuncOp) attempting to erase consecutive CNOTs under certain conditions.
  // The conditions are:
  // 1. The two CNOTs should appear consecutively
  // 2. There should not be any intervening operation between the CNOTs that
  // operating on
  //    the Input/Output Qubits of the CNOTs.
  // 3. The two CNOTs should operate on the same Input Qubits
  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonGateCancellationPass]\n";

    auto KernelDialectInfo = analysis.getKernelDialectInfo();

    // Empty CompareKey meaning - both control and target qubit operands
    // of the gates to be cancelled will be compared
    Comparety CompareKey;

    for(auto &[kernel, Info] : KernelDialectInfo){
      performCancellation(Info, GatesToCancel, CompareKey);
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

class CommonNullRotationCancellation
    : public PassWrapper<CommonNullRotationCancellation, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonNullRotationCancellation)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonNullRotationCancellationPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass removes the gates RX, RY, RZ having NULL rotation angles.";
  }

  std::vector<Gate> RotationGatesToCancel{RX, RY, RZ};
 
  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonGateCancellationPass]\n";

    auto KernelDialectInfo = analysis.getKernelDialectInfo();

    for(auto &[kernel, Info] : KernelDialectInfo){
      performNullRotationCancellation(Info, RotationGatesToCancel);
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};


} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonGateCancellationPass() {
  return std::make_unique<CommonGateCancellation>();
}

std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonNullRotationCancellationPass() {
  return std::make_unique<CommonNullRotationCancellation>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonGateCancellationPass() {
  return std::make_unique<CommonGateCancellation>();
}
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonNullRotationCancellationPass() {
  return std::make_unique<CommonNullRotationCancellation>();
}
#endif