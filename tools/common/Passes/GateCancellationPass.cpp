

#include "../SemanticExtractLayer/Extractor.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#ifdef BUILD_CUDAQ_ENABLED
#include "MQSSCUDAQPasses/Transforms.hpp"
#endif
#ifdef BUILD_CATALYST_ENABLED
#include "MQSSCatalystPasses/Transforms.hpp"
#endif

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

  void runOnOperation() override {

    // TODO: Think about how we can run the analysis once and reuse results
    auto &analysis = getAnalysis<MyModuleAnalysis>();
    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Common GateCancellationPass]:\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    for (auto kernel : kernels) {
      std::vector<Operation *> kernelCNOTs;
      auto OpView = analysis.getDialectInfo().OpQuantumView;
      kernel->walk([&](Operation *Op) {
        if (OpView.count(Op)) {
          auto gatety = OpView[Op].GateTy;
          if (gatety == "CNOT")
            kernelCNOTs.push_back(Op);
        }
      });

      SmallSetVector<Operation *, 16> ToErase;
      for (int i = 0; i < kernelCNOTs.size(); i++) {
        auto CNOT1 = kernelCNOTs.at(i);
        if(ToErase.count(CNOT1))
          continue;
        // Starting from CNOT1, check all intervening operations until CNOT2.
        // The intervening operations should not touch any of the Qubits used by 
        // CNOT1.
        for (int j = i+1; j < kernelCNOTs.size(); j++) {
          auto CNOT2 = kernelCNOTs.at(j);

          auto CNOT1View = OpView[CNOT1];
          auto CNOT2View = OpView[CNOT2];

          auto *nextOp = CNOT1->getNextNode();

          bool touches = false;
          while (nextOp != CNOT2) {

            auto nextOpView = OpView[nextOp];
      
            if (nextOpView.hasSideEffects &&
                (analysis.touchesAny(nextOp, CNOT1View.InputQubits) ||
                 analysis.touchesAny(nextOp, CNOT1View.OutputQubits))) {
              touches = true;
              break;
            }
            nextOp = nextOp->getNextNode();
          }

          // If an intervening Op touches CNOT1's Qubits, stop the analysis
          if (touches) {
            continue;
          }

          // If both CNOT1 and CNOT2 operate on the same Qubits, they can be erased
          if (analysis.sameQubits(
                  {CNOT1View.InputQubits, CNOT1View.OutputQubits},
                  {CNOT2View.InputQubits, CNOT2View.OutputQubits})) {
            ToErase.insert(CNOT1);
            ToErase.insert(CNOT2);
          }
        }
      }

      for(auto *Op : ToErase){
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