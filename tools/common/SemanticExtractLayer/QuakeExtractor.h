

#include "../../mqss-cudaq/include/Utils/utils.h"
#include "Extractor.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

class QuakeAnalysis : public MyModuleAnalysis {

public:
  QuakeAnalysis(ModuleOp module) : module(module) { gatherOpInfo(); }

  SmallVector<func::FuncOp, 16> fetchQuantumKernels() override {

    SmallVector<func::FuncOp, 16> QuantumKernels;
    auto walkResult = module.walk([&](Operation *op) {
      // Check if it is a quantum kernel
      // TODO (Akshay): Check here for catalyst kernel?
      if (auto funcOp = dyn_cast<func::FuncOp>(op)) {

        if (funcOp->hasAttr(cudaq::entryPointAttrName)) {
          QuantumKernels.push_back(funcOp);
          return WalkResult::advance();
        }
        for (auto arg : funcOp.getArguments()) {
          if (isa<quake::RefType, quake::VeqType>(arg.getType())) {
            QuantumKernels.push_back(funcOp);
            return WalkResult::advance();
          }
        }

        // Skip functions which are not quantum kernels
        return WalkResult::skip();
      }

      // Check if it is controlled quake.apply
      if (auto applyOp = dyn_cast<quake::ApplyOp>(op))
        if (!applyOp.getControls().empty())
          return WalkResult::interrupt();

      return WalkResult::advance();
    });

    return QuantumKernels;
    //   return std::make_tuple(Info.QuantumKernels, walkResult);
  }

  // Gather Information (QuantumView) about every operation
  void gatherOpInfo() override {

    auto QuantumKernels = fetchQuantumKernels();

    for (auto kernel : QuantumKernels) {

      QuantumOpInfo QInfo;
      kernel.getBody().walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quake") {
          auto view = createQuantumView(Op);
          QInfo[Op] = view;
        }
      });
      KernelDialectInfo[kernel] = QInfo;
    }
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }

  void addOperation(Operation *NewOp) override {
    auto funcOp = NewOp->getParentOfType<mlir::func::FuncOp>();
    assert(funcOp && "Adding New O: Parent Function not found!");
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp];
    assert(!QInfo.count(NewOp) &&
           "Adding New Op: Op already present in QunatumInfoMap");
    auto view = createQuantumView(NewOp);
    QInfo[NewOp] = view;
  }

  const llvm::DenseMap<func::FuncOp, QuantumOpInfo>
  getKernelDialectInfo() override {
    return KernelDialectInfo;
  }

  const QuantumOpView getOpInfo(Operation *Op) override {
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    assert(KernelDialectInfo.count(funcOp) &&
           "Get Op Info: No QuantumOpInfo for funcOp");
    auto QInfo = KernelDialectInfo[funcOp];
    assert(QInfo.count(Op) && "Get Op Info: Op not present QuantumInfoMap");
    return QInfo[Op];
  }

private:
  ModuleOp module;
  llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo;

  const QuantumOpView createQuantumView(Operation *Op) {

    QuantumOpView view;
    if (!mlir::isMemoryEffectFree(Op)) {
      view.hasSideEffects = true;
    }

    if (auto gate = dyn_cast<quake::OperatorInterface>(Op)) {
      // Gather the Control Qubits
      view.isAdjoint = gate.isAdj();

      for (auto ctrl : gate.getControls()) {
        if (auto ext_ref =
                dyn_cast<quake::ExtractRefOp>(ctrl.getDefiningOp())) {
          QubitID ID;
          Value base = ext_ref.getVeq();
          auto index = ext_ref.getConstantIndex();
          ID.base = base;
          ID.index = index;
          view.getQubits(QubitRole::Control).ids.push_back(ID);
          view.getQubits(QubitRole::Control).in.push_back(ext_ref);
        }
      }
      // Gather the Target Qubits
      for (auto t : gate.getTargets()) {
        if (auto ext_ref = dyn_cast<quake::ExtractRefOp>(t.getDefiningOp())) {
          QubitID ID;
          Value base = ext_ref.getVeq();
          auto index = ext_ref.getConstantIndex();
          ID.base = base;
          ID.index = index;
          view.getQubits(QubitRole::Target).ids.push_back(ID);
          view.getQubits(QubitRole::Target).in.push_back(ext_ref);
        }
      }
      // Gather Parameters if any (E.g. Rotation Angle)
      for (auto p : gate.getParameters()) {
        view.params.push_back(p);
      }
      auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
      if (isQGateOp) {
        view.GateTy = parseGateTy(GateTy);
      }
    } else if (auto extract_refop = dyn_cast<quake::ExtractRefOp>(Op)) {
      // Do something
      QubitID ID;
      ID.base = extract_refop.getVeq();
      ID.index = extract_refop.getConstantIndex();
      view.getQubits(QubitRole::Target).ids.push_back(ID);
    }
    return view;
  }
};
