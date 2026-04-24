


#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "Extractor.h"


#include "../../mqss-cudaq/include/Utils/utils.h"



class QuakeAnalysis : public MyModuleAnalysis {

public:
  QuakeAnalysis(ModuleOp module) : module(module) {
    gatherOpInfo();
  }

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
          QuantumOpView OpView;
          if (!mlir::isMemoryEffectFree(Op)){
            OpView.hasSideEffects = true;
          }

          if (auto gate = dyn_cast<quake::OperatorInterface>(Op)) {
            // Gather the Control Qubits
            OpView.isAdj = gate.isAdj();
            for (auto ctrl : gate.getControls()) {
              if (auto ext_ref =
                      dyn_cast<quake::ExtractRefOp>(ctrl.getDefiningOp())) {
                QubitID ID;
                Value base = ext_ref.getVeq();
                auto index = ext_ref.getConstantIndex();
                ID.base = base;
                ID.index = index;
                OpView.ControlInQubits.push_back(ID);
                OpView.ControlInQubitOps.push_back(ext_ref);
              }
            }
            // Gather the Target Qubits
            for (auto t : gate.getTargets()) {
              if (auto ext_ref =
                      dyn_cast<quake::ExtractRefOp>(t.getDefiningOp())) {
                QubitID ID;
                Value base = ext_ref.getVeq();
                auto index = ext_ref.getConstantIndex();
                ID.base = base;
                ID.index = index;
                OpView.TargetInQubits.push_back(ID);
                OpView.TargetInQubitOps.push_back(ext_ref);
              }
            }
            // Gather Parameters if any (E.g. Rotation Angle)
            for(auto p : gate.getParameters()){
              OpView.Params.push_back(p);
            }

            auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
            if (isQGateOp){
              OpView.GateTy = parseGateTy(GateTy);
            }
          }
          else if(auto extract_refop = dyn_cast<quake::ExtractRefOp>(Op)){
            //Do something
            QubitID ID;
            ID.base = extract_refop.getVeq();
            ID.index = extract_refop.getConstantIndex();
            OpView.TargetInQubits.push_back(ID);
          }
          QInfo[Op] = OpView;
        }
        
      });
      KernelDialectInfo[kernel] = QInfo;
    }
  }

  mlir::LogicalResult verifyModule() override{
    return module.verify();
    
  }

  const llvm::DenseMap<func::FuncOp, QuantumOpInfo>  getKernelDialectInfo() override {
    return KernelDialectInfo;
  }

private:
  ModuleOp module;
  llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo;
};

