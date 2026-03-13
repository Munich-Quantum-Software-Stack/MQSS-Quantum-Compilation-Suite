


#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include "Extractor.h"
#include "../../mqss-catalyst/include/Utils/utils.h"

enum class QubitRole {
    Control,
    Target
};

static const std::map<StringRef, std::vector<QubitRole>> gateOperandRoleTable = {
    // Controlled gates
    {"CNOT", {QubitRole::Control, QubitRole::Target}},
    {"CX",   {QubitRole::Control, QubitRole::Target}},
    {"CY",   {QubitRole::Control, QubitRole::Target}},
    {"CZ",   {QubitRole::Control, QubitRole::Target}},

    // Single-qubit gates
    {"PauliX", {QubitRole::Target}},
    {"PauliY", {QubitRole::Target}},
    {"PauliZ", {QubitRole::Target}},
    {"X", {QubitRole::Target}},   // sometimes emitted
    {"Y", {QubitRole::Target}},
    {"Z", {QubitRole::Target}},
    {"Hadamard", {QubitRole::Target}},
    {"H", {QubitRole::Target}},

    // Rotations
    {"RX", {QubitRole::Target}},
    {"RY", {QubitRole::Target}},
    {"RZ", {QubitRole::Target}},

    // Two-qubit symmetric gates
    {"SWAP", {QubitRole::Target, QubitRole::Target}}
    //TODO:Add more Here
};

static const std::vector<QubitRole> getGateOpRoles(const StringRef &gateName)
{
    auto it = gateOperandRoleTable.find(gateName);
    if (it != gateOperandRoleTable.end())
        return it->second;

    return {};
}


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
            OpView.GateTy = parseGateTy(g.getGateName());

            if (auto mem = dyn_cast<MemoryEffectOpInterface>(Op))
              if (!mem.hasNoEffect())
                OpView.hasSideEffects = true;

            std::vector<QubitRole> OpRoles = getGateOpRoles(g.getGateName()); // Now we can separate out the Qubits into Ctrl/Target
            assert(!OpRoles.empty() && "Found a gate Op with empty Operand Roles(Control/Target)");
            assert((OpRoles.size() == g.getQubitOperands().size()) && "Operand Roles not equals No. of Qubit Operands");

            for (unsigned i = 0; i < g.getQubitOperands().size(); i++) {
              QubitID ID;
              QubitRole role = OpRoles[i];
              
              if(role == QubitRole::Control){
                ID.base = g->getOperand(i);
                ID.index = -1;
                OpView.ControlQubits.push_back(ID);
              }
              if(role == QubitRole::Target){
                ID.base = g->getOperand(i);
                ID.index = -1;
                OpView.TargetQubits.push_back(ID);
              }
              
            }
            
          }
        }
        Info.OpQuantumView[Op] = OpView;
      });
    }
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