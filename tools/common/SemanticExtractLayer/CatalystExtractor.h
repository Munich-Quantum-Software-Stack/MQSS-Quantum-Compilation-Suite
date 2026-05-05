
#include "../../mqss-catalyst/include/Utils/utils.h"
#include "Extractor.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

// static const StringSet<> rotationsSet = {"RX", "RY", "RZ"};
// static const StringSet<> hermitianSet = {"Hadamard", "PauliX", "PauliY",
// "PauliZ",
//                                          "H",        "X",      "Y", "Z"};
// static const StringSet<> multiQubitSet = {"CNOT", "CZ", "SWAP"};

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
        {"RX", {QubitRole::Target}},
        {"RY", {QubitRole::Target}},
        {"RZ", {QubitRole::Target}},

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
  CatalystQuantumAnalysis(ModuleOp module) : module(module) { gatherOpInfo(); }

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

      QuantumOpInfo QInfo;
      kernel.getBody().walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quantum") {

          auto view = createQuantumView(Op);
          QInfo[Op] = view;
        }
      });
      KernelDialectInfo[kernel] = QInfo;
    }
  }

  void addOperation(Operation *NewOp) override{
    auto funcOp = getOpParentFunc(NewOp);
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp];
    assert(!QInfo.count(NewOp) && "Adding New Op: Op already present in QunatumInfoMap");
    auto view = createQuantumView(NewOp);
    QInfo[NewOp] = view;
  }

  bool UpdateOperands(Operation *Op, QubitRole Role, Value OrigValue,
                      Value NewValue) {
    auto funcOp = getOpParentFunc(Op);
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    QuantumOpInfo &QInfo = KernelDialectInfo[funcOp];
    assert(QInfo.count(Op) && "Updating Op: Op not present in QunatumInfoMap");
    QuantumOpView &OpQView = QInfo[Op];
    auto &Operands = OpQView.getQubits(Role).in;
    auto it = std::find(Operands.begin(), Operands.end(), OrigValue);
    if (it == Operands.end()) {
      return false; // originalValue not found
    }
    *it = NewValue;
    return true;
  }

  llvm::DenseMap<func::FuncOp, QuantumOpInfo> getKernelDialectInfo() override {
    return KernelDialectInfo;
  }

  const QuantumOpView getOpInfo(Operation *Op) override{
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    assert(KernelDialectInfo.count(funcOp) && "Get Op Info: No QuantumOpInfo for funcOp");
    auto QInfo = KernelDialectInfo[funcOp];
    assert(QInfo.count(Op) && "Get Op Info: Op not present QuantumInfoMap");
    return QInfo[Op];
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }

private:
  ModuleOp module;
  llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo;

  QuantumOpView createQuantumView(Operation *Op) {
    QuantumOpView view;
    if (hasQuantumEffect(Op)) {
      view.hasSideEffects = true;
    }
    // Only consider operations on gates with side-effects
    if (auto g = isCatalystQuantumGateOp(Op)) {
      // TODO isAdj initialization?
      view.GateTy = parseGateTy(g.getGateName());
      view.isAdjoint = g.getAdjointFlag(); // Is this correct?

      std::vector<QubitRole> OpRoles =
          getGateOpRoles(g.getGateName()); // Now we can separate out the
                                           // Qubits into Ctrl/Target

      assert(!OpRoles.empty() &&
             "Found a gate Op with empty Operand Roles(Control/Target)");
      assert((OpRoles.size() == g.getQubitOperands().size()) &&
             "Operand Roles not equals No. of Qubit Operands");

      for (auto p : g.getParams()) {
        view.params.push_back(p);
      }

      for (unsigned i = 0; i < g.getQubitOperands().size(); i++) {
        QubitID ID;
        QubitRole role = OpRoles[i];

        auto gop = g.getQubitOperands()[i];
        auto resop = g->getResults()[i];

        if (role == QubitRole::Control) {
          ID.base = gop;
          ID.index = -1;
          view.getQubits(QubitRole::Control).ids.push_back(ID);
          view.getQubits(QubitRole::Control).in.push_back(gop);
          view.getQubits(QubitRole::Control).out.push_back(resop);
        }
        if (role == QubitRole::Target) {
          ID.base = gop;
          ID.index = -1;
          view.getQubits(QubitRole::Target).ids.push_back(ID);
          view.getQubits(QubitRole::Target).in.push_back(gop);
          view.getQubits(QubitRole::Target).out.push_back(resop);
        }
      }
    }
    // llvm::outs() << "Op: " << *Op << "\n";
    // for(auto c : OpView.ControlQubits){
    //   llvm::outs().indent(4) << " Ctrl: " << c.base << "\n";
    // }
    //  for(auto t : OpView.TargetQubits){
    //   llvm::outs().indent(4) << " t: " << t.base << "\n";
    // }
    return view;
  }

  mlir::func::FuncOp getOpParentFunc(Operation *Op){
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    assert(funcOp && "Adding New O: Parent Function not found!");
    return funcOp;
  }
};
