


#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "include/utils.h"

using namespace mlir;
using namespace llvm;

namespace {

class CommonCxCancellation
    : public PassWrapper<CommonCxCancellation, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCxCancellation)

  [[nodiscard]] StringRef getArgument() const override {
    return "CommonCxCancellation";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass removes the pattern CNot, CNot if both gates operates on "
           "the same control and targets.";
  }

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

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Applying Pass: CommonCxCancellation]\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      performCancellation(curr_op, OpQuantumView, Gate::CNOT);
   
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonCxCancellationPass() {
  return std::make_unique<CommonCxCancellation>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonCxCancellationPass() {
  return std::make_unique<CommonCxCancellation>();
}
#endif