
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "Pass.h"

using namespace mlir;

struct CommuteTy {
  Operation *Op1;
  Operation *Op2;

public:
  CommuteTy(Operation *Op1, Operation *Op2) : Op1(Op1), Op2(Op2) {}
};

static void Commute(std::vector<CommuteTy> CommmutationCandidates) {

  for (auto [Cand1, Cand2] : CommmutationCandidates) {
    llvm::outs() << "Commuting...\n";
    llvm::outs().indent(4) << "Cand 1: " << *Cand1 << "\n";
    llvm::outs().indent(4) << "Cand 2: " << *Cand2 << "\n";
    Cand1->moveAfter(Cand2);

    llvm::outs().indent(4) << "---------------------------------------------\n";
  }
}

static void cancel(Operation *Op) {
  mlir::IRRewriter rewriter(Op->getContext());
  // Erase the operations
  rewriter.eraseOp(Op);
}

static bool equivalence_check(const std::vector<QubitID> &Op1,
                              const std::vector<QubitID> &Op2) {
  if (Op1.size() != Op2.size())
    return false;

  for (const auto &q1 : Op1) {
    bool found = false;
    for (const auto &q2 : Op2) {
      if (q1.base == q2.base && q1.index == q2.index) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  return true;
}

static bool sameQubits(tupleVectorsQubitIDs Op1CtrlTarget,
                       tupleVectorsQubitIDs Op2CtrlTarget) {

  auto &[Op1Ctrls, Op1Targets] = Op1CtrlTarget;
  auto &[Op2Ctrls, Op2Targets] = Op2CtrlTarget;

  if (!Op1Ctrls.empty() && !Op2Ctrls.empty()) {
    auto CtrlCheck = equivalence_check(Op1Ctrls, Op2Ctrls);
    if (!CtrlCheck)
      return false;
  }

  auto TargetCheck = equivalence_check(Op1Targets, Op2Targets);

  return TargetCheck;
}

static bool isContained(QubitID KeyQubit, std::vector<QubitID> QubitList) {

  for (auto ctrl : QubitList) {
    if (ctrl.base == KeyQubit.base && ctrl.index == KeyQubit.index)
      return true;
  }
  return false;
}

static bool
touchesAny(Operation *Op2, std::vector<QubitID> Op1QubitIDs,
           std::unordered_map<Operation *, QuantumOpView> OpQuantumView) {

  for (auto Op1Qubit : Op1QubitIDs) {
    auto Op1Qubitbase = Op1Qubit.base;
    auto Op1Qubitidx = Op1Qubit.index;

    for (auto Op2op : Op2->getOperands()) {
      if ((Op2op == Op1Qubitbase)) {
        return true;
      } else {
        if (OpQuantumView.count(Op2op.getDefiningOp())) {
          auto OpView = OpQuantumView[Op2op.getDefiningOp()];
          for (auto ctrl : OpView.ControlQubits) {
            if (ctrl.base == Op1Qubitbase && ctrl.index == Op1Qubitidx)
              return true;
          }
          if (isContained(Op1Qubit, OpView.ControlQubits) ||
              isContained(Op1Qubit, OpView.TargetQubits))
            return true;
        }
        // return false;
      }
    }
  }
  return false;
}

static std::vector<CommuteTy>
performCommutation(Operation *curr_op,
                   std::unordered_map<Operation *, QuantumOpView> OpQuantumView,
                   Gate FirstGateTy, Gate SecondGateTy) {

  std::vector<CommuteTy> CommmuteCandidates;
  while (curr_op) {

    auto curr_op_qView = OpQuantumView[curr_op];
    // llvm::outs() << "curr op: " << *curr_op << "\n";
    // Only continue with the analysis if current op is a CNOT
    if (curr_op_qView.GateTy != FirstGateTy) {
      curr_op = curr_op->getNextNode();
      continue;
    }

    auto *next_op = curr_op->getNextNode();
    if (!next_op)
      break;

    while (next_op) {
      auto nextOpView = OpQuantumView[next_op];
      // llvm::outs() << "next op: " << *next_op << "\n";
      if (nextOpView.hasSideEffects && nextOpView.GateTy != SecondGateTy &&
          (touchesAny(next_op, curr_op_qView.ControlQubits, OpQuantumView) ||
           touchesAny(next_op, curr_op_qView.TargetQubits, OpQuantumView))) {

        // llvm::outs().indent(4) << "Found intervening ops!\n";
        break;
      }

      if (nextOpView.GateTy == SecondGateTy) {

        if (sameQubits(
                {curr_op_qView.ControlQubits, curr_op_qView.TargetQubits},
                {nextOpView.ControlQubits, nextOpView.TargetQubits})) {
          CommuteTy comm{curr_op, next_op};
          CommmuteCandidates.emplace_back(comm);
        }
        // llvm::outs().indent(4) << "Not same Qubits!\n";
        break;
      }

      next_op = next_op->getNextNode();
    }

    curr_op = curr_op->getNextNode();
  }

  if (!CommmuteCandidates.empty()){
    Commute(CommmuteCandidates);
  }
  else
    llvm::outs() << "No Commutation Candidates!!\n";

  return CommmuteCandidates;
}

static void performCommuteAndSwitch(Operation *curr_op,
                   std::unordered_map<Operation *, QuantumOpView> OpQuantumView,
                   Gate FirstGateTy, Gate SecondGateTy, Gate ReplaceGateTy){

    auto CommutedOps = performCommutation(curr_op, OpQuantumView, FirstGateTy, SecondGateTy);
    for(auto &[Op1, Op2] : CommutedOps){
    createAndEraseGate(Op2, parseGateTy(ReplaceGateTy));
    }
}

static void performCancellation(
    Operation *curr_op,
    std::unordered_map<Operation *, QuantumOpView> OpQuantumView,
    Gate GateToCancel) {
  // Iterate operation-by-operation starting from the first operation
  // in the kernel
  SmallSetVector<Operation *, 16> ToErase;
  while (curr_op) {

    auto curr_op_qView = OpQuantumView[curr_op];
    // Only continue with the analysis if current op is a CNOT
    if (curr_op_qView.GateTy != GateToCancel) {
      curr_op = curr_op->getNextNode();
      continue;
    }

    auto *next_op = curr_op->getNextNode();
    if (!next_op)
      break;

    // Now, iterate operation-by-operation:
    // 1. If an intervening Non-CNOT operation, with side-effects is found:
    // abandon
    // 2. If a CNOT is found, check if the two CNOTs operate on the same
    // Qubits
    //    - If yes, the two CNOTs can be erased
    //    - Otherwise, abandon
    while (next_op) {

      auto nextOpView = OpQuantumView[next_op];
      // TODO: Should the "touchesAny" check be there?
      if (nextOpView.hasSideEffects && (nextOpView.GateTy != GateToCancel) &&
          (touchesAny(next_op, curr_op_qView.ControlQubits, OpQuantumView) ||
           touchesAny(next_op, curr_op_qView.TargetQubits, OpQuantumView))) {
        break;
      }

      if (nextOpView.GateTy == GateToCancel) {

        if (sameQubits(
                {curr_op_qView.ControlQubits, curr_op_qView.TargetQubits},
                {nextOpView.ControlQubits, nextOpView.TargetQubits})) {
          ToErase.insert(curr_op);
          ToErase.insert(next_op);
          next_op = next_op->getNextNode();
        }

        break;
      }

      next_op = next_op->getNextNode();
    }

    curr_op = curr_op->getNextNode();
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->Erasing: " << *Op << "\n";
    cancel(Op);
  }
}