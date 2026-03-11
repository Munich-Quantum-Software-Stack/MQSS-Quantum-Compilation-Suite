


#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include "Extractor.h"
#include "../../mqss-catalyst/include/Utils/utils.h"


class CatalystQuantumAnalysis : public MyModuleAnalysis {

public:
  CatalystQuantumAnalysis(ModuleOp module) : module(module) {
    fetchQuantumKernels();
    gatherOpInfo();
  }

  void fetchQuantumKernels() override {

    auto walkResult = module.walk([&](Operation *op) {
      // Check if it is a quantum kernel
      // TODO (Akshay): Check here for catalyst kernel?
      if (auto funcOp = dyn_cast<func::FuncOp>(op)) {
        // TODO: Is this a solid check?
        funcOp.walk([&](Operation *fop) {
          if (fop->getDialect()->getNamespace() == "quantum") {
            Info.QuantumKernels.push_back(funcOp);
            return WalkResult::interrupt();
          }
          return WalkResult::advance();
        });

        // Skip functions which are not quantum kernels
        return WalkResult::skip();
      }
      return WalkResult::advance();
    });
  }

  void gatherOpInfo() override {
    for (auto kernel : Info.QuantumKernels) {

      kernel.getBody().walk([&](Operation *Op) {
        QuantumOpView OpView;
        if (!mlir::isMemoryEffectFree(Op))
          OpView.hasSideEffects = true;
        if (Op->getDialect()->getNamespace() == "quantum") {

          // Only consider operations on gates with side-effects
          if (auto g = isCatalystQuantumGateOp(Op)) {
            if (auto mem = dyn_cast<MemoryEffectOpInterface>(Op))
              if (!mem.hasNoEffect())
                OpView.hasSideEffects = true;
            for (unsigned i = 0; i < g.getNumOperands(); i++) {
              QubitID ID;
              ID.base = g->getOperand(i);
              ID.index = -1;
              OpView.InputQubits.push_back(ID);
            }

            for (unsigned i = 0; i < g->getNumResults(); i++) {
              QubitID ID;
              ID.base = g->getResult(i);
              ID.index = -1;
              OpView.OutputQubits.push_back(ID);
            }
            OpView.GateTy = g.getGateName();
          }
        }
        Info.OpQuantumView[Op] = OpView;
      });
    }
  }

  bool touchesAny(Operation *Op2, std::vector<QubitID> Op1QubitIDs) override {
    for (auto op : Op2->getOperands()) {
      // Catalyst case
      for (auto Op1Qubit : Op1QubitIDs) {
        if (Op1Qubit.index == -1) {
          if (op == Op1Qubit.base)
            return true; // Return if an operand Qubit of Op1 is used in Op2
        }
      }
    }

    return false;
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }

  std::vector<Operation *> getGateOps() override {
    if (Info.QuantumKernels.empty()) {
      llvm::outs() << "Empty kernels\n";
      return {};
    }

    for (auto kernel : Info.QuantumKernels) {
      kernel->walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quantum") {
          if (isCatalystQuantumGateOp(Op))
            Info.GateOps.push_back(Op);
        }
      });
    }
    return Info.GateOps;
  }

private:
  ModuleOp module;
};

extern std::unique_ptr<CatalystQuantumAnalysis> analysis;

struct CatalystQuantumAnalysisPass
    : public mlir::PassWrapper<CatalystQuantumAnalysisPass,
                               mlir::OperationPass<mlir::ModuleOp>> {

  void runOnOperation() override {
    auto module = getOperation();
    analysis = std::make_unique<CatalystQuantumAnalysis>(module);
  }
};