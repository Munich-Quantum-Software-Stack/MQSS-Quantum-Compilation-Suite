

#include "include/PassUtils.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonGateCancellation
    : public mqss_backend::CommonGateCancellationPassBase<CommonGateCancellation> {
      
  std::vector<Gate> GatesToCancel{CNOT, PauliX, PauliZ, PauliY, H, Hadamard};

  std::vector<Gate> RotationGatesToCancel{RX, RZ, RZ};
  // The following algorithm, iterates over operations in an
  // mlir-kernel (FuncOp) attempting to erase consecutive CNOTs under
  // certain conditions. The conditions are:
  // 1. The two CNOTs should appear consecutively
  // 2. There should not be any intervening operation between the
  // CNOTs that operating on
  //    the Input/Output Qubits of the CNOTs.
  // 3. The two CNOTs should operate on the same Input Qubits
  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.

    llvm::outs() << "\n[Applying Pass: CommonGateCancellationPass]\n";

    auto &analysis = getAnalysis<DialectAnalysis>();
    auto KernelDialectInfo = analysis.getKernelDialectInfo();

    // Empty CompareKey meaning - both control and target qubit operands
    // of the gates to be cancelled will be compared

    if (mode == "CancelGate") {
      Comparety CompareKey;

      for (auto &[kernel, Info] : KernelDialectInfo) {
        performCancellation(Info, GatesToCancel, CompareKey);
      }
    }
    else if(mode == "CancelNullRotation"){
      for (auto &[kernel, Info] : KernelDialectInfo)
        performNullRotationCancellation(Info, RotationGatesToCancel);
    } else{
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


std::unique_ptr<mlir::Pass> mqss_backend::CommonGateCancellationPass() {
  return std::make_unique<CommonGateCancellation>();
}
