

#include "Pass.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;


namespace {

class GateCommutation
    : public PassWrapper<GateCommutation, OperationPass<mlir::ModuleOp>> {

public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(GateCommutation)

  [[nodiscard]] StringRef getArgument() const override {
    return "MyGateCommutationPass";
  }
  [[nodiscard]] StringRef getDescription() const override {
    return "This pass searches for a specific pattern where operation T2 is"
           "followed by T1 and attempts to commute them to the order T1 "
           "followed by T2,"
           "under the constraints of input and output qubit counts.";
  }

  void Commute(){
    
  }

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    auto OpQuantumView = analysis.getDialectInfo().OpQuantumView;

    llvm::outs() << "\n[Common GateCommutationPass]:\n";

    auto kernels = analysis.getDialectInfo().QuantumKernels;

    auto FirstGateTy = Gate::CNOT;
    auto SecondGateTy = Gate::RX;

    for (auto kernel : kernels) {

      auto ops = kernel.getFunctionBody().getOps().begin();
      auto *curr_op = &*ops;

      while (curr_op) {

        auto curr_op_qView = OpQuantumView[curr_op];
        // Only continue with the analysis if current op is a CNOT
        if (curr_op_qView.GateTy != FirstGateTy) {
          curr_op = curr_op->getNextNode();
          continue;
        }

        auto *next_op = curr_op->getNextNode();
        if (!next_op)
          break;

        while(next_op){
          auto nextOpView = OpQuantumView[next_op];

          if (nextOpView.hasSideEffects &&  nextOpView.GateTy!=SecondGateTy &&
              (analysis.touchesAny(next_op, curr_op_qView.ControlQubits) ||
               analysis.touchesAny(next_op, curr_op_qView.TargetQubits))) {
            // llvm::outs().indent(6) << "has side-effects\n";
            break;
          }

          if(nextOpView.GateTy == SecondGateTy){

            llvm::outs() << "Found second target: " << *next_op << "\n";
            for(auto InQubit : nextOpView.ControlQubits){
                llvm::outs().indent(4) << "Ctrl: " << InQubit.base << "," << InQubit.index << "\n";
            }
             for(auto OutQubit : nextOpView.TargetQubits){
                llvm::outs().indent(4) << "Target: " << OutQubit.base << "," << OutQubit.index << "\n";
            }
            
            break;
          }

          next_op = next_op->getNextNode();
        }

        curr_op = next_op;
      }


    }
  }



};
} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::GateCommutationPass() {
  return std::make_unique<GateCommutation>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> catalyst::opt::GateCommutationPass() {
  return std::make_unique<GateCommutation>();
}
#endif