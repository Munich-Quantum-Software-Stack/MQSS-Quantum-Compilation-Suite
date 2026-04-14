
#include "PassIncludes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include <cmath>

using namespace mlir;

struct Comparety {
  std::string KeyGate1 = "";
  std::string KeyGate2 = "";
};

struct PassInfoty {
  std::vector<Gate> FirstGateTy;
  std::vector<Gate> SecondGateTy;
  std::unordered_map<Gate, Gate> ReplacementMap;
  Comparety CompareKey;
};

struct ReductionPassInfoty {
  std::vector<Gate> GatesToCancel;
  Gate NewGateTy;
  Comparety CompareKey;
};

struct CommuteTy {
  Operation *Op1 = nullptr;
  Operation *Op2 = nullptr;
};

struct CommuteInfoTy {

public:
  void gather(Operation *Op1, Operation *Op2) {
    CommuteTy Commute{Op1, Op2};
    CommuteCandidates.push_back(Commute);
  }

  bool isScheduled(Operation *KeyOp) {
    if (CommuteCandidates.empty())
      return false;

    for (auto Cand : CommuteCandidates) {
      if (Cand.Op1 == KeyOp || Cand.Op2 == KeyOp)
        return true;
    }
    return false;
  }

  std::vector<CommuteTy> getCommutationCandidates() {
    return CommuteCandidates;
  }

private:
  std::vector<CommuteTy> CommuteCandidates;
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

static std::vector<Value> getQubitValues(std::vector<QubitID> QubitVector) {
  std::vector<Value> QubitValues;
  for (auto v : QubitVector)
    QubitValues.push_back(v.base);
  return QubitValues;
  ;
}

static bool checkDoublePiMultiplies(double angle) {
  const double pi = std::numbers::pi;
  const double doublePi = 2 * pi;
  if (std::fmod(angle, doublePi) == 0)
    return true;
  return false;
}

static void cancel(Operation *Op) {
  mlir::IRRewriter rewriter(Op->getContext());
  // Erase the operations
  rewriter.eraseOp(Op);
}

static Value normalizeValue(double param, mlir::IRRewriter &builder,
                            Location loc) {

  double pi = std::numbers::pi;
  param =
      param - (std::floor(param / (2 * pi)) * 2 * pi); // normalize the angle
  auto valueAttr = builder.getFloatAttr(builder.getF64Type(), param);
  auto constantOp = builder.create<mlir::arith::ConstantOp>(loc, valueAttr);
  return constantOp.getResult();
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

static bool SameQubits(tupleVectorsQubitIDs Gate1CtrlTarget,
                       tupleVectorsQubitIDs Gate2CtrlTarget,
                       Comparety Comparekey) {

  auto &[Gate1Ctrls, Gate1Targets] = Gate1CtrlTarget;
  auto &[Gate2Ctrls, Gate2Targets] = Gate2CtrlTarget;

  if (Comparekey.KeyGate1 == "Control" && Comparekey.KeyGate2 == "Target") {
    return equivalence_check(Gate1Ctrls, Gate2Targets);
  }
  if (Comparekey.KeyGate1 == "Target" && Comparekey.KeyGate2 == "Control") {
    return equivalence_check(Gate1Targets, Gate2Ctrls);
  }
  if (Comparekey.KeyGate1 == "Target" && Comparekey.KeyGate2 == "Target") {
    return equivalence_check(Gate1Targets, Gate2Targets);
  }
  return (equivalence_check(Gate1Ctrls, Gate2Ctrls) &&
          equivalence_check(Gate1Targets, Gate2Targets));
}

static bool isContained(QubitID KeyQubit, std::vector<QubitID> QubitList) {

  for (auto ctrl : QubitList) {
    if (ctrl.base == KeyQubit.base && ctrl.index == KeyQubit.index)
      return true;
  }
  return false;
}

static bool touchesAny(Operation *Op2, std::vector<QubitID> Op1QubitIDs,
                       std::map<Operation *, QuantumOpView> OpQuantumView) {

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

static Operation *createNewGate(Location loc, llvm::StringRef NewGateTy,
                                std::vector<Value> ControlQubitOps,
                                std::vector<Value> TargetQubitOps,
                                const mlir::ValueRange params,
                                mlir::IRRewriter &builder, bool isAdj = false) {

  Operation *NewOp;
#ifdef BUILD_CUDAQ_ENABLED
  NewOp = createQuakeGate(loc, NewGateTy, ControlQubitOps, TargetQubitOps,
                          params, builder, isAdj);
#endif
#ifdef BUILD_CATALYST_ENABLED
  NewOp = createCatalystGate(loc, NewGateTy, ControlQubitOps, TargetQubitOps,
                             params, builder, isAdj);
#endif
  return NewOp;
}

static std::vector<CommuteTy>
performCommutation(std::map<Operation *, QuantumOpView> OpQuantumView,
                   PassInfoty PassInfo) {

  auto Gate1Ty = PassInfo.FirstGateTy;
  auto Gate2Ty = PassInfo.SecondGateTy;
  auto CompareKey = PassInfo.CompareKey;

  CommuteInfoTy CommutationInfo;
  for (auto &[FirstGateOp, FirstGateOpQView] : OpQuantumView) {
    auto FirstGateTy = FirstGateOpQView.GateTy;

    if (!FirstGateOpQView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
      continue;

    if (!is_contained(Gate1Ty, FirstGateTy))
      continue;

    if (CommutationInfo.isScheduled(FirstGateOp))
      continue;

    auto *NextOp = FirstGateOp->getNextNode();
    if (!NextOp)
      continue;

    while (NextOp) {
      auto nextOpView = OpQuantumView[NextOp];
      // llvm::outs() << "next op: " << *NextOp << "\n";
      if (nextOpView.hasSideEffects &&
          !is_contained(Gate2Ty, nextOpView.GateTy) &&
          (touchesAny(NextOp, FirstGateOpQView.ControlQubits, OpQuantumView) ||
           touchesAny(NextOp, FirstGateOpQView.TargetQubits, OpQuantumView))) {

        // llvm::outs().indent(4) << "Found intervening ops!\n";
        break;
      }

      if (is_contained(Gate2Ty, nextOpView.GateTy)) {

        if (SameQubits(
                {FirstGateOpQView.ControlQubits, FirstGateOpQView.TargetQubits},
                {nextOpView.ControlQubits, nextOpView.TargetQubits},
                CompareKey)) {
          CommutationInfo.gather(FirstGateOp, NextOp);
        }
        // llvm::outs().indent(4) << "Not same Qubits!\n";
        break;
      }

      NextOp = NextOp->getNextNode();
    }
  }

  auto CommuteCandidates = CommutationInfo.getCommutationCandidates();
  if (!CommuteCandidates.empty()) {
    Commute(CommuteCandidates);
  } else
    llvm::outs() << "No Commutation Candidates!!\n";

  return CommuteCandidates;
}

static void
performCommuteAndSwitch(std::map<Operation *, QuantumOpView> OpQuantumView,
                        PassInfoty PassInfo) {

  auto CommutedOps = performCommutation(OpQuantumView, PassInfo);

  for (auto &[Op1, Op2] : CommutedOps) {
    if (OpQuantumView.count(Op2)) {
      auto Op2TargetQubits = OpQuantumView[Op2].TargetQubits;
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
      std::vector<Value> targetQubitsbaseVector;
      for (auto Op : OpToSwitchOut->getOperands()) {
        targetQubitsbaseVector.push_back(Op);
      }

      createNewGate(Op2->getLoc(), ReplacementGateTy, {},
                    targetQubitsbaseVector, {}, builder);

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
    auto parameters = FirstGateQView.Params;
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

    while (NextOp) {

      auto nextOpView = OpQuantumView[NextOp];
      // TODO: Should the "touchesAny" check be there?
      if (nextOpView.hasSideEffects && (nextOpView.GateTy != FirstGateTy) &&
          (touchesAny(NextOp, FirstGateQView.ControlQubits, OpQuantumView) ||
           touchesAny(NextOp, FirstGateQView.TargetQubits, OpQuantumView))) {
        break;
      }

      if (nextOpView.GateTy == FirstGateTy) {

        if (SameQubits(
                {FirstGateQView.ControlQubits, FirstGateQView.TargetQubits},
                {nextOpView.ControlQubits, nextOpView.TargetQubits},
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

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
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
        while (NextOp != SecondPatternOp) {
          auto nextOpView = OpQuantumView[NextOp];
          if (nextOpView.hasSideEffects &&
              (touchesAny(NextOp, FirstPatternOpQView.ControlQubits,
                          OpQuantumView) ||
               touchesAny(NextOp, FirstPatternOpQView.TargetQubits,
                          OpQuantumView))) {
            InterveningOps = true;
            break;
          }

          NextOp = NextOp->getNextNode();
        }

        if (InterveningOps) {
          ChecksSatisfied = false;
          break;
        }

        if (!SameQubits({FirstPatternOpQView.ControlQubits,
                         FirstPatternOpQView.TargetQubits},
                        {SecondPatternOpQView.ControlQubits,
                         SecondPatternOpQView.TargetQubits},
                        PassInfo.CompareKey)) {
          ChecksSatisfied = false;
          break;
        }
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
    auto RefOp = ToErase[ToErase.size() - 1];
    llvm::outs() << "Ref gate: " << *RefOp << "\n";

    std::vector<Value> targetQubitsbaseVector;
    for (auto Op : RefOp->getOperands()) {
      targetQubitsbaseVector.push_back(Op);
      llvm::outs() << "Switch out target: " << Op << "\n";
    }

    mlir::IRRewriter builder(RefOp->getContext());
    builder.setInsertionPointAfter(RefOp);

    auto ReOpParams = OpQuantumView[RefOp].Params;

    auto RefOpCtrlQubits = OpQuantumView[RefOp].ControlQubits;
    auto RefOptargetQubits = OpQuantumView[RefOp].TargetQubits;

    auto isAdj = OpQuantumView[RefOp].isAdj;
    auto *NewOp =
        createNewGate(RefOp->getLoc(), parseGateTy(PassInfo.NewGateTy), {},
                      targetQubitsbaseVector, {}, builder, isAdj);

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
    for (auto param : GateQView.Params) {
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

    auto GateCtrlQubits = getQubitValues(GateQView.ControlQubits);
    auto GateTargetQubits = getQubitValues(GateQView.TargetQubits);

    auto *NewOp = createNewGate(GateOp->getLoc(), parseGateTy(GateQView.GateTy),
                                GateCtrlQubits, GateTargetQubits,
                                GateQView.Params, builder, GateQView.isAdj);

    llvm::outs() << "--->Created: " << *NewOp << "\n";
  }

  for (auto *Op : ToErase) {
    llvm::outs() << "-->To Erase: " << *Op << "\n";
    cancel(Op);
  }
}