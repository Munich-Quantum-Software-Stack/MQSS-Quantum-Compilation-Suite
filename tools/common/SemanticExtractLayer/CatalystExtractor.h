
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "../../mqss-catalyst/include/Utils/utils.h"
#include "Extractor.h"

#include "llvm/Support/raw_ostream.h"

enum class QubitRole { Control, Target, Rotation };

static const std::map<StringRef, std::vector<QubitRole>> gateOperandRoleTable =
    {
        // Controlled gates
        {"CNOT", {QubitRole::Control, QubitRole::Target}},
        {"CX", {QubitRole::Control, QubitRole::Target}},
        {"CY", {QubitRole::Control, QubitRole::Target}},
        {"CZ", {QubitRole::Control, QubitRole::Target}},

        // Single-qubit gates
        {"PauliX", {QubitRole::Target}},
        {"PauliY", {QubitRole::Target}},
        {"PauliZ", {QubitRole::Target}},
        {"Hadamard", {QubitRole::Target}},
        {"H", {QubitRole::Target}},

        // Rotations
        {"RX", {QubitRole::Rotation, QubitRole::Target}},
        {"RY", {QubitRole::Rotation, QubitRole::Target}},
        {"RZ", {QubitRole::Rotation, QubitRole::Target}},

        // Two-qubit symmetric gates
        {"SWAP", {QubitRole::Target, QubitRole::Target}}
        // TODO:Add more Here
};

static const std::vector<QubitRole> getGateOpRoles(const StringRef &gateName) {
  auto it = gateOperandRoleTable.find(gateName);
  if (it != gateOperandRoleTable.end())
    return it->second;

  return {};
}

class CatalystQuantumAnalysis : public MyModuleAnalysis {

public:
  CatalystQuantumAnalysis(ModuleOp module) : module(module){
    gatherOpInfo();
  }

  SmallVector<func::FuncOp, 16> fetchQuantumKernels() override {

    SmallVector<func::FuncOp, 16> QuantumKernels;
    auto walkResult = module.walk([&](Operation *op) {
      // Check if it is a quantum kernel
      // TODO (Akshay): Check here for catalyst kernel?
      if (auto funcOp = dyn_cast<func::FuncOp>(op)) {
        // TODO: Is this a solid check?
        funcOp.walk([&](Operation *fop) {
          if (fop->getDialect()->getNamespace() == "quantum") {
            QuantumKernels.push_back(funcOp);
            return WalkResult::interrupt();
          }
          return WalkResult::advance();
        });

        // Skip functions which are not quantum kernels
        return WalkResult::skip();
      }
      return WalkResult::advance();
    });
    return QuantumKernels;
  }

  void gatherOpInfo() override {
    auto QuantumKernels = fetchQuantumKernels();

    for (auto kernel : QuantumKernels) {

      kernel.getBody().walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quantum") {

          QuantumOpView OpView;
          if (hasQuantumEffect(Op)){
            OpView.hasSideEffects = true;
          }

          // Only consider operations on gates with side-effects
          if (auto g = isCatalystQuantumGateOp(Op)) {
            OpView.GateTy = parseGateTy(g.getGateName());

            std::vector<QubitRole> OpRoles =
                getGateOpRoles(g.getGateName()); // Now we can separate out the
                                                 // Qubits into Ctrl/Target
            assert(!OpRoles.empty() &&
                   "Found a gate Op with empty Operand Roles(Control/Target)");
            assert((OpRoles.size() == g.getOperands().size()) &&
                   "Operand Roles not equals No. of Qubit Operands");

            for (unsigned i = 0; i < g.getOperands().size(); i++) {
              QubitID ID;
              QubitRole role = OpRoles[i];

              if (role == QubitRole::Control) {
                ID.base = g->getOperand(i);
                ID.index = -1;
                OpView.ControlQubits.push_back(ID);
              }
              if (role == QubitRole::Target) {
                ID.base = g->getOperand(i);
                ID.index = -1;
                OpView.TargetQubits.push_back(ID);
              }
            }
          }
          // llvm::outs() << "Op: " << *Op << "," << OpView.hasSideEffects << ", gatety:" << OpView.GateTy << "\n";
          Info.OpQuantumView[Op] = OpView;
        }
      });
      KernelDialectInfo[kernel] = Info;
    }
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }

private:
  ModuleOp module;
};
