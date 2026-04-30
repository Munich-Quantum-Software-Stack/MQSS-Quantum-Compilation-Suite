
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include <cmath>
#include "PassIncludes.h"


static std::vector<CommuteTy>
performCommutation(std::map<Operation *, QuantumOpView> QView,
                   PassInfoty PassInfo) {

  auto Gate1Ty = PassInfo.FirstGateTy;
  auto Gate2Ty = PassInfo.SecondGateTy;
  auto CompareKey = PassInfo.CompareKey;

  CommuteInfoTy CommutationInfo;
  CommuteInfoTy SSACommuteCandidates;
  for (auto &[Op1, Op1QView] : QView) {
    auto FirstGateTy = Op1QView.GateTy;

    if (!Op1QView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
      continue;

    if (!is_contained(Gate1Ty, FirstGateTy))
      continue;

    if (CommutationInfo.isScheduled(Op1))
      continue;

    auto *Op2 = Op1->getNextNode();
    if (!Op2)
      continue;

    auto Op1CtrlQubits = Op1QView.getQubits(QubitRole::Control);
    auto Op1TargetQubits = Op1QView.getQubits(QubitRole::Target);

    while (Op2) {
      auto nextOpView = QView[Op2];

      auto Op2CtrlQubits = nextOpView.getQubits(QubitRole::Control);
      auto Op2TargetQubits = nextOpView.getQubits(QubitRole::Target);
      // llvm::outs() << "next op: " << *NextOp << "\n";
      if (nextOpView.hasSideEffects &&
          !is_contained(Gate2Ty, nextOpView.GateTy) &&
          (touchesAny(Op2, Op1CtrlQubits.ids, QView) ||
           touchesAny(Op2, Op1TargetQubits.ids, QView))) {

        // llvm::outs().indent(4) << "Found intervening ops!\n";
        break;
      }

      if (is_contained(Gate2Ty, nextOpView.GateTy)) {

        if (SameQubits({Op1CtrlQubits.ids, Op1TargetQubits.ids},
                       {Op2CtrlQubits.ids, Op2TargetQubits.ids},
                       CompareKey)) {
          CommutationInfo.gather(Op1, Op2);
        } 
        else if (SameQubitValues(
                       {Op1CtrlQubits.out, Op1TargetQubits.out},
                       {Op2CtrlQubits.in, Op2TargetQubits.in},
                       CompareKey)) {
          // Example:
          // Original:
          //  %out_qubits_2:2 = quantum.custom "CNOT"() %2, %3 : !quantum.bit,
          //  !quantum.bit %out_ctrl = quantum.custom "RX"(%cst) %out_qubits_2#0
          //  : !quantum.bit
          // Commuted:
          //  %new_ctrl = quantum.custom "RX"(%cst) %2 : !quantum.bit
          //  %new_pair:2 = quantum.custom "CNOT"() %new_ctrl, %3 :
          //  !quantum.bit, !quantum.bit
          // llvm::outs() << "Gathering for SSA:\n";
          // llvm::outs() << "Op1: " << *Op1 << "\n";
          // llvm::outs() << "Op2: " << *Op2 << "\n";
          SSACommuteCandidates.gather(Op1, Op2, Op1QView, nextOpView);
        }
        // TODO: For catalyst - Cannot commute by checking the output qubits
        // and input qubits of Gate Ops. If the check returns true, and the
        // gates are commuted, it will break SSA form (Def-Use will be commuted
        // to Use-Def). llvm::outs().indent(4) << "Not same Qubits!\n";
        break;
      }

      Op2 = Op2->getNextNode();
    }
  }

  auto CommuteCandidates = CommutationInfo.getCommutationCandidates();
  if (!CommuteCandidates.empty()) {
    Commute(CommuteCandidates);
  } else {
    auto SSACommuteCands = SSACommuteCandidates.getCommutationCandidates();
    if (!SSACommuteCands.empty()) {
      Commute(SSACommuteCands, CompareKey);
    }
    else {
      llvm::outs().indent(4) << "-----No Commutation candidates for the kernel----\n";
    }
  }

  return CommuteCandidates;
}

static void
performCommuteAndSwitch(std::map<Operation *, QuantumOpView> OpQuantumView,
                        PassInfoty PassInfo) {

  auto CommutedOps = performCommutation(OpQuantumView, PassInfo);

  for (auto Ops : CommutedOps) {
    auto *Op1 = Ops.Op1;
    auto *Op2 = Ops.Op2;
    if (OpQuantumView.count(Op2)) {
      auto Op2TargetQubits = OpQuantumView[Op2].getQubits(QubitRole::Target).ids;
      assert(Op2TargetQubits.size() == 1 &&
             "Currently, Can only switch single qubit gates!");

      mlir::IRRewriter builder(Op2->getContext());
      builder.setInsertionPointAfter(Op2);

      // Since gate creation has dialect specific semantics, need to have
      //  dialect specific APIs to create the gates.
      //  TODO: 1. Can we do better? Do not like the use of MACROS
      //        2. What if Op1 is to be switched out and replaced?

      auto Op1Ty = OpQuantumView[Op1].GateTy;
      auto Op2Ty = OpQuantumView[Op2].GateTy;

      Operation *OpToSwitchOut = nullptr;
      StringRef ReplacementGateTy = "";
      for (auto &[Gatety, ReplaceWithTy] : PassInfo.ReplacementMap) {
        if (Gatety == Op1Ty) {
          OpToSwitchOut = Op1;
          ReplacementGateTy = parseGateTy(ReplaceWithTy);
          break;
        } else if (Gatety == Op2Ty) {
          OpToSwitchOut = Op2;
          ReplacementGateTy = parseGateTy(ReplaceWithTy);
          break;
        }
      }

      assert(OpToSwitchOut && "Op to switch out cannot be NULL!!");
      assert(!ReplacementGateTy.empty() && "Need the replacementgatety!!");

      auto TargetInQubitOps =
          OpQuantumView[OpToSwitchOut].getQubits(QubitRole::Target).in;

      createNewGate(OpToSwitchOut, ReplacementGateTy, {}, TargetInQubitOps, {}, builder);

      builder.eraseOp(OpToSwitchOut);
    }
  }
}

static void performNullRotationCancellation(
    std::map<Operation *, QuantumOpView> OpQuantumView,
    std::vector<Gate> GatesToCancel) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, FirstGateQView] : OpQuantumView) {

    auto GateTy = FirstGateQView.GateTy;

    if (!is_contained(GatesToCancel, GateTy)) {
      // It is assumed that GatesToCancel will contain : RX, RY, RZ
      continue;
    }
    auto parameters = FirstGateQView.params;
    bool deleteGate = true;
    for (auto param : parameters) {
      if (auto constOp = param.getDefiningOp<mlir::arith::ConstantOp>()) {
        if (auto floatAttr = dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double v = floatAttr.getValueAsDouble();
          if (!checkDoublePiMultiplies(v) && v != 0) {
            deleteGate = false;
            break;
          }
        }
      }
    }

    if (deleteGate)
      ToErase.insert(GateOp);
  }

  for (auto gop : ToErase) {
    llvm::outs() << "--->Erasing: " << *gop << "\n";
    cancel(gop);
  }
}

static void
performCancellation(std::map<Operation *, QuantumOpView> OpQuantumView,
                    std::vector<Gate> GatesToCancel, Comparety CompareKey) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[FirstGateOp, FirstGateQView] : OpQuantumView) {

    auto FirstGateTy = FirstGateQView.GateTy;
    if (!FirstGateQView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
      continue;

    if (!is_contained(GatesToCancel, FirstGateTy))
      continue;

    if (ToErase.count(FirstGateOp))
      continue;

    auto *NextOp = FirstGateOp->getNextNode();
    if (!NextOp)
      continue;

    auto FirstGateCtrlQubits = FirstGateQView.getQubits(QubitRole::Control);
    auto FirstGateTargetQubits = FirstGateQView.getQubits(QubitRole::Target);

    while (NextOp) {

      auto nextOpView = OpQuantumView[NextOp];
      auto nextGateCtrlQubits = nextOpView.getQubits(QubitRole::Control);
      auto nextGateTargetQubits = nextOpView.getQubits(QubitRole::Target);
      // TODO: Should the "touchesAny" check be there?
      if (nextOpView.hasSideEffects && (nextOpView.GateTy != FirstGateTy) &&
          (touchesAny(NextOp, FirstGateCtrlQubits.ids, OpQuantumView) ||
           touchesAny(NextOp, FirstGateTargetQubits.ids, OpQuantumView))) {
        break;
      }

      if (nextOpView.GateTy == FirstGateTy) {

        if (SameQubits({FirstGateCtrlQubits.ids, FirstGateTargetQubits.ids},
                       {nextGateCtrlQubits.ids, nextGateTargetQubits.ids},
                       CompareKey)) {
          ToErase.insert(FirstGateOp);
          ToErase.insert(NextOp);
          NextOp = NextOp->getNextNode();
        } else if (SameQubitValues(
                       {FirstGateCtrlQubits.out, FirstGateTargetQubits.out},
                       {nextGateCtrlQubits.in, nextGateTargetQubits.in},
                       CompareKey)) {
          ToErase.insert(FirstGateOp);
          ToErase.insert(NextOp);
          NextOp = NextOp->getNextNode();
        }

        break;
      }

      NextOp = NextOp->getNextNode();
    }
  }

  for (auto *op : ToErase) {
    llvm::outs() << "--> Erasing: " << *op << "\n";

    for (unsigned i = 0; i < op->getNumResults(); ++i) {
      op->getResult(i).replaceAllUsesWith(op->getOperand(i));
    }

    cancel(op);
  }
}

struct pipelinety {
  Operation *GateOp;
  QuantumOpView GateOpView;
};

static void performReduction(std::map<Operation *, QuantumOpView> OpQuantumView,
                             const ReductionPassInfoty PassInfo) {

  SmallSetVector<Operation *, 16> ToErase;

  int i = 0;
  auto FirstGateToCancelTy = PassInfo.GatesToCancel[i];

  std::vector<pipelinety> pipeline;

  for (auto &[GateOp, FirstGateQView] : OpQuantumView) {

    auto FirstGateTy = FirstGateQView.GateTy;
    if (!FirstGateQView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
      continue;

    if (is_contained(PassInfo.GatesToCancel, FirstGateTy) &&
        pipeline.size() != PassInfo.GatesToCancel.size()) {
      pipeline.push_back({GateOp, FirstGateQView});
    }

    if (pipeline.size() == PassInfo.GatesToCancel.size()) {
      bool patterncheck = true;
      for (auto i = 0; i < pipeline.size(); i++) {
        if (pipeline[i].GateOpView.GateTy != PassInfo.GatesToCancel[i]) {
          break;
        }
      }

      if (!patterncheck) {
        pipeline.clear();
        continue;
      }

      bool ChecksSatisfied = true;
      for (int i = 0; i < pipeline.size() - 1; i++) {
        auto FirstPatternOp = pipeline[i].GateOp;
        auto FirstPatternOpQView = pipeline[i].GateOpView;
        auto SecondPatternOp = pipeline[i + 1].GateOp;
        auto SecondPatternOpQView = pipeline[i].GateOpView;

        auto NextOp = FirstPatternOp->getNextNode();

        bool InterveningOps = false;
        auto FirstGateCtrlQubits = FirstGateQView.getQubits(QubitRole::Control);
        auto FirstGateTargetQubits =
            FirstGateQView.getQubits(QubitRole::Target);

        auto SecondGateCtrlQubits =
            SecondPatternOpQView.getQubits(QubitRole::Control);
        auto SecondGateTargetQubits =
            SecondPatternOpQView.getQubits(QubitRole::Target);

        while (NextOp != SecondPatternOp) {
          auto nextOpView = OpQuantumView[NextOp];
          if (nextOpView.hasSideEffects &&
              (touchesAny(NextOp, FirstGateCtrlQubits.ids, OpQuantumView) ||
               touchesAny(NextOp, FirstGateTargetQubits.ids, OpQuantumView))) {
            InterveningOps = true;
            break;
          }

          NextOp = NextOp->getNextNode();
        }

        if (InterveningOps) {
          ChecksSatisfied = false;
          break;
        }

        if (!SameQubits({FirstGateCtrlQubits.ids, FirstGateTargetQubits.ids},
                        {SecondGateCtrlQubits.ids, SecondGateTargetQubits.ids},
                        PassInfo.CompareKey)) {
          ChecksSatisfied = false;
          break;
        }
        // TODO: For catalyst - Check equivalence b/w result Qubits of FirstOp
        // and OpQubits of Next Op
        //      See performCancellation for reference
      }

      if (ChecksSatisfied) {
        for (auto [GOp, View] : pipeline)
          ToErase.insert(GOp);
      }
      pipeline.clear();
    }
  }

  if (PassInfo.NewGateTy != Gate::UNKNOWN) {

    std::vector<Value> QubitsbaseVector;

    // TODO: Is it correct to take this as the RefOp?
    auto RefOp = ToErase[ToErase.size() - 1];
    llvm::outs() << "Ref gate: " << *RefOp << "\n";

    mlir::IRRewriter builder(RefOp->getContext());
    builder.setInsertionPointAfter(RefOp);

    auto RefOpParams = OpQuantumView[RefOp].params;

    auto RefOpCtrlQubits =
        OpQuantumView[RefOp].getQubits(QubitRole::Control).in;
    auto RefOptargetQubits =
        OpQuantumView[RefOp].getQubits(QubitRole::Target).in;

    auto isAdj = OpQuantumView[RefOp].isAdjoint;
    auto *NewOp =
        createNewGate(RefOp, parseGateTy(PassInfo.NewGateTy), RefOpCtrlQubits,
                      RefOptargetQubits, RefOpParams, builder, isAdj);

    llvm::outs() << "--->Created: " << *NewOp << "\n";
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
  }
}

static void performArgAngelNormalization(
    std::map<Operation *, QuantumOpView> OpQuantumView) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, GateQView] : OpQuantumView) {
    if (GateQView.GateTy != Gate::RX && GateQView.GateTy != Gate::RY &&
        GateQView.GateTy != Gate::RZ) {
      continue;
    }

    mlir::IRRewriter builder(GateOp->getContext());
    builder.setInsertionPointAfter(GateOp);

    std::vector<mlir::Value> nParameters;
    for (auto param : GateQView.params) {
      if (auto constOp = param.getDefiningOp<mlir::arith::ConstantOp>()) {
        if (auto floatAttr = dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double v = floatAttr.getValueAsDouble();

          auto newVal = normalizeValue(v, builder, GateOp->getLoc());

          if (newVal != param)
            nParameters.push_back(newVal);
        }
      }
    }

    if (nParameters.empty())
      continue;

    ToErase.insert(GateOp);
    ValueRange normalizedParams(nParameters);

    auto GateCtrlQubits = GateQView.getQubits(QubitRole::Control).in;
    auto GateTargetQubits = GateQView.getQubits(QubitRole::Target).in;

    auto *NewOp = createNewGate(GateOp, parseGateTy(GateQView.GateTy),
                                GateCtrlQubits, GateTargetQubits,
                                GateQView.params, builder, GateQView.isAdjoint);

    llvm::outs() << "--->Created: " << *NewOp << "\n";
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
  }
}

static void
performCNOTReversal(std::map<Operation *, QuantumOpView> OpQuantumView) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, GateQView] : OpQuantumView) {
    if ((GateQView.GateTy != Gate::CNOT))
      continue;

    mlir::IRRewriter builder(GateOp->getContext());
    builder.setInsertionPointAfter(GateOp);

    auto ControlInQubitOps = GateQView.getQubits(QubitRole::Control).in;
    auto TargetInQubitOps = GateQView.getQubits(QubitRole::Target).in;
    createNewGate(GateOp, "H", {}, TargetInQubitOps, GateQView.params, builder);

    createNewGate(GateOp, "H", {}, ControlInQubitOps, GateQView.params,
                  builder);

    createNewGate(GateOp, "CNOT", TargetInQubitOps, ControlInQubitOps,
                  GateQView.params, builder);

    createNewGate(GateOp, "H", {}, TargetInQubitOps, GateQView.params, builder);

    createNewGate(GateOp, "H", {}, ControlInQubitOps, GateQView.params,
                  builder);
    ToErase.insert(GateOp);
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
  }
}

static void
performDecomposition(std::map<Operation *, QuantumOpView> OpQuantumView,
                     bool ReverseCNOT = false) {

  if (ReverseCNOT) {
    performCNOTReversal(OpQuantumView);
    return;
  }
  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, GateQView] : OpQuantumView) {
    if ((GateQView.GateTy != Gate::CNOT) && (GateQView.GateTy != Gate::CZ))
      continue;

    mlir::IRRewriter builder(GateOp->getContext());
    builder.setInsertionPointAfter(GateOp);

    llvm::outs() << "Decomposing CNOT\n";
    auto ControlInQubitOps = GateQView.getQubits(QubitRole::Control).in;
    auto TargetInQubitOps = GateQView.getQubits(QubitRole::Target).in;

    if (GateQView.GateTy == Gate::CNOT) {
      createNewGate(GateOp, "H", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      createNewGate(GateOp, "CZ", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      createNewGate(GateOp, "H", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      ToErase.insert(GateOp);
    } else {
      createNewGate(GateOp, "H", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      createNewGate(GateOp, "CNOT", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      createNewGate(GateOp, "H", ControlInQubitOps, TargetInQubitOps,
                    GateQView.params, builder);
      ToErase.insert(GateOp);
    }
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
  }
}