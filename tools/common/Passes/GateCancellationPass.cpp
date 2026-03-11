


#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include "Pass.h"

using namespace mlir;
using namespace llvm;

namespace {

class GateCancellation
    : public PassWrapper<GateCancellation, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(GateCancellation)

  [[nodiscard]] StringRef getArgument() const override {
    return "MyGateCancellationPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass removes the pattern CNot, CNot if both gates operates on "
           "the same control and targets.";
  }

  void cancel(Operation *Op) {
    mlir::IRRewriter rewriter(Op->getContext());
    // Erase the operations
    rewriter.eraseOp(Op);
  }


  // The following algorithm, iterates over operations in an mlir-kernel (FuncOp)
  // attempting to erase consecutive CNOTs under certain conditions. The conditions are:
  // 1. The two CNOTs should appear consecutively
  // 2. There should not be any intervening operation between the CNOTs that operating on
  //    the Input/Output Qubits of the CNOTs.
  // 3. The two CNOTs should operate on the same Input Qubits
  void runOnOperation() override {

    // TODO: Think about how we can run the analysis once and reuse results
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Common GateCancellationPass]:\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      SmallSetVector<Operation *, 16> ToErase;
      // Iterate operation-by-operation starting from the first operation
      // in the kernel
      while (curr_op) {

        auto curr_op_qView = OpQuantumView[curr_op];
        // Only continue with the analysis if current op is a CNOT
        if (curr_op_qView.GateTy != "CNOT") {
          curr_op = curr_op->getNextNode();
          continue;
        }

        auto *next_op = curr_op->getNextNode();
        if (!next_op)
          break;

        // Now, iterate operation-by-operation:
        // 1. If an intervening Non-CNOT operation, with side-effects is found: abandon
        // 2. If a CNOT is found, check if the two CNOTs operate on the same Qubits
        //    - If yes, the two CNOTs can be erased
        //    - Otherwise, abandon
        while (next_op) {

          auto nextOpView = OpQuantumView[next_op];

          if (nextOpView.hasSideEffects && nextOpView.GateTy != "CNOT" &&
              (analysis.touchesAny(next_op, curr_op_qView.InputQubits) ||
               analysis.touchesAny(next_op, curr_op_qView.OutputQubits))) {
            // llvm::outs().indent(6) << "has side-effects\n";
            break;
          }

          if (nextOpView.GateTy == "CNOT") {

            if (analysis.sameQubits(
                    {curr_op_qView.InputQubits, curr_op_qView.OutputQubits},
                    {nextOpView.InputQubits, nextOpView.OutputQubits})) {
              // llvm::outs().indent(6) << "Matched\n";
              ToErase.insert(curr_op);
              ToErase.insert(next_op);
              next_op = next_op->getNextNode();
            }

            break;
          }

          next_op = next_op->getNextNode();
        }

        curr_op = next_op;
      }

      for (auto *Op : ToErase) {
        llvm::outs() << "-->Erasing: " << *Op << "\n";
        cancel(Op);
      }
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::GateCancellationPass() {
  return std::make_unique<GateCancellation>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> catalyst::opt::GateCancellationPass() {
  return std::make_unique<GateCancellation>();
}
#endif