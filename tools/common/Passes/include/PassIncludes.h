

#ifdef BUILD_CUDAQ_ENABLED
#include "MQSSCUDAQPasses/Pipelines.h"
#include "MQSSCUDAQPasses/Transforms.hpp"
#include "SemanticExtractLayer/QuakeExtractor.h"
namespace mqss_backend = mqss_cudaq::opt;

#define DialectAnalysis QuakeAnalysis

namespace mqss_cudaq::opt {
#define GEN_PASS_CLASSES
#include "MQSSCUDAQPasses/Transforms.h.inc"

} // namespace mqss_cudaq::opt
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "MQSSCatalystPasses/Pipelines.h"
#include "MQSSCatalystPasses/Transforms.h"
#include "SemanticExtractLayer/CatalystExtractor.h"

namespace mqss_backend = mqss_catalyst::opt;
#define DialectAnalysis CatalystQuantumAnalysis

namespace mqss_catalyst::opt {
#define GEN_PASS_CLASSES
#include "MQSSCatalystPasses/Transforms.h.inc"

} // namespace mqss_catalyst::opt
#endif

using namespace mlir;
using namespace llvm;

struct Comparety {
  QubitRole KeyGate1;
  QubitRole KeyGate2;
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
  QuantumOpView Op1QView;
  QuantumOpView Op2QView;
};

struct CommuteInfoTy {

public:
  void gather(Operation *Op1, Operation *Op2) {
    CommuteTy Commute{Op1, Op2};
    CommuteCandidates.push_back(Commute);
  }
  void gather(Operation *Op1, Operation *Op2, QuantumOpView Op1QView,
              QuantumOpView Op2QView) {
    CommuteTy Commute{Op1, Op2, Op1QView, Op2QView};
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


static std::vector<Value> getQubitValues(std::vector<QubitID> QubitVector) {
  std::vector<Value> QubitValues;
  for (auto v : QubitVector)
    QubitValues.push_back(v.base);
  return QubitValues;
  ;
}

static bool checkDoublePiMultiplies(double angle) {
  const double pi = numbers::pi;
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

  double pi = numbers::pi;
  param =
      param - (std::floor(param / (2 * pi)) * 2 * pi); // normalize the angle
  auto valueAttr = builder.getFloatAttr(builder.getF64Type(), param);
  auto constantOp = builder.create<mlir::arith::ConstantOp>(loc, valueAttr);
  return constantOp.getResult();
}

static bool equivalence_check(SmallVector<QubitID, 2> &Op1,
                              SmallVector<QubitID, 2> &Op2) {
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

static bool equivalence_check(const SmallVector<mlir::Value, 2> &Op1,
                              const SmallVector<mlir::Value, 2> &Op2) {
  if (Op1.size() != Op2.size())
    return false;

  for (const auto &q1 : Op1) {
    bool found = false;
    for (const auto &q2 : Op2) {
      if (q1 == q2) {
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

  if (Comparekey.KeyGate1 == QubitRole::Control &&
      Comparekey.KeyGate2 == QubitRole::Target) {
    return equivalence_check(Gate1Ctrls, Gate2Targets);
  }
  if (Comparekey.KeyGate1 == QubitRole::Target &&
      Comparekey.KeyGate2 == QubitRole::Control) {
    return equivalence_check(Gate1Targets, Gate2Ctrls);
  }
  if (Comparekey.KeyGate1 == QubitRole::Target &&
      Comparekey.KeyGate2 == QubitRole::Target) {
    return equivalence_check(Gate1Targets, Gate2Targets);
  }
  return (equivalence_check(Gate1Ctrls, Gate2Ctrls) &&
          equivalence_check(Gate1Targets, Gate2Targets));
}

static bool SameQubitValues(tupleVectorsValues Gate1CtrlTarget,
                            tupleVectorsValues Gate2CtrlTarget,
                            Comparety Comparekey) {

  auto &[Gate1Ctrls, Gate1Targets] = Gate1CtrlTarget;
  auto &[Gate2Ctrls, Gate2Targets] = Gate2CtrlTarget;

  if (Comparekey.KeyGate1 == QubitRole::Control &&
      Comparekey.KeyGate2 == QubitRole::Target) {
    return equivalence_check(Gate1Ctrls, Gate2Targets);
  }
  if (Comparekey.KeyGate1 == QubitRole::Target &&
      Comparekey.KeyGate2 == QubitRole::Control) {
    return equivalence_check(Gate1Targets, Gate2Ctrls);
  }
  if (Comparekey.KeyGate1 == QubitRole::Target &&
      Comparekey.KeyGate2 == QubitRole::Target) {
    return equivalence_check(Gate1Targets, Gate2Targets);
  }
  return (equivalence_check(Gate1Ctrls, Gate2Ctrls) &&
          equivalence_check(Gate1Targets, Gate2Targets));
}

static bool isContained(QubitID KeyQubit, SmallVector<QubitID, 2> QubitList) {

  for (auto ctrl : QubitList) {
    if (ctrl.base == KeyQubit.base && ctrl.index == KeyQubit.index)
      return true;
  }
  return false;
}

static bool touchesAny(Operation *Op2, SmallVector<QubitID, 2> Op1QubitIDs,
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
          auto ControlInQubits = OpView.getQubits(QubitRole::Control);
          for (auto ctrl : ControlInQubits.ids) {
            if (ctrl.base == Op1Qubitbase && ctrl.index == Op1Qubitidx)
              return true;
          }
          auto TargetInQubits = OpView.getQubits(QubitRole::Target);
          if (isContained(Op1Qubit, ControlInQubits.ids) ||
              isContained(Op1Qubit, TargetInQubits.ids))
            return true;
        }
        // return false;
      }
    }
  }
  return false;
}

static Operation *createNewGate(Operation *OpToReplace,
                                llvm::StringRef NewGateTy,
                                SmallVector<mlir::Value, 2> ControlQubitOps,
                                SmallVector<mlir::Value, 2> TargetQubitOps,
                                const mlir::ValueRange params,
                                mlir::IRRewriter &builder, bool isAdj = false) {

  Operation *NewOp;
#ifdef BUILD_CUDAQ_ENABLED
  NewOp = createQuakeGate(OpToReplace->getLoc(), NewGateTy, ControlQubitOps,
                          TargetQubitOps, params, builder, isAdj);
#endif
#ifdef BUILD_CATALYST_ENABLED
  NewOp = createCatalystGate(OpToReplace, NewGateTy, ControlQubitOps,
                             TargetQubitOps, params, builder, isAdj);
#endif
  return NewOp;
}

static void Commute(std::vector<CommuteTy> CommmutationCandidates) {

  for (auto CandidatesInfo : CommmutationCandidates) {
    llvm::outs() << "Commuting...\n";
    auto *Cand1 = CandidatesInfo.Op1;
    auto *Cand2 = CandidatesInfo.Op2;
    llvm::outs().indent(4) << "Cand 1: " << *Cand1 << "\n";
    llvm::outs().indent(4) << "Cand 2: " << *Cand2 << "\n";
    Cand1->moveAfter(Cand2);

    llvm::outs().indent(4) << "---------------------------------------------\n";
  }
}

static void Commute(std::vector<CommuteTy> CommmutationCandidates,
                    Comparety CompareKey) {

  auto KeyGate1 = CompareKey.KeyGate1;
  auto KeyGate2 = CompareKey.KeyGate2;

  for (auto OpInfo : CommmutationCandidates) {

    llvm::outs().indent(4) << "SSA Commuting...\n";

    auto *Op1 = OpInfo.Op1;
    auto *Op2 = OpInfo.Op2;
    auto Op1QView = OpInfo.Op1QView;
    auto Op2QView = OpInfo.Op2QView;

    llvm::outs().indent(4) << "Op1: " << *Op1 << "\n";
    llvm::outs().indent(4) << "Op2: " << *Op2 << "\n";

    // Get the Op1 and Op2 Qubits being compared
    // E.g. commuting CNOT (Op1) and Rx (Op2)
    Op1->moveAfter(Op2);
    // Now, CNOT is after Rx
    //auto Op1Operand = OpInfo.Op1QView.getQubits(KeyGate1).in[0];

    // Replace operand (Ctrl/Target) of CNOT with the result of Rx (Ctrl/Target)
    // The CompareKeys are used to determine which operand (Ctrl/Target) is replaced

    assert(!Op1QView.getQubits(KeyGate1).in.empty() && "Op1 does not have operand targets");
    assert(!Op2QView.getQubits(KeyGate2).out.empty() && "Op2 does not have result targets");

    auto Op1Operand = Op1QView.getQubits(KeyGate1).in[0];
    auto Op2Operand = Op2QView.getQubits(KeyGate2).in[0];

    auto Op2Result = Op2QView.getQubits(KeyGate2).out[0];
    
    // Do something with the Operand of Rx (Op2)
    Op2->replaceUsesOfWith(Op2Operand, Op1Operand);
    Op1->replaceUsesOfWith(Op1Operand, Op2Result);

    llvm::outs().indent(4) << "---------------------------------------------\n";

  }

  // auto cnot = ...; // old CNOT
  // auto rx = ...;   // old RX

  // Value ctrlIn = cnot->getOperand(0);
  // Value tgtIn = cnot->getOperand(1);
  // Value theta =
  //     rx->getOperand(0); // or attribute/param extraction, depending on op
  //     form

  // auto newRx = rewriter.create<... RX...>(loc,
  // /*resultType=*/ctrlIn.getType(),
  //                                         theta, ctrlIn);
  // auto newCnot = rewriter.create<... CNOT...>(
  //     loc, TypeRange{ctrlIn.getType(), tgtIn.getType()},
  //     ValueRange{newRx.getResult(), tgtIn});

  // // Redirect final outputs of old subgraph:
  // rx->getResult(0).replaceAllUsesWith(newCnot->getResult(0));
  // cnot->getResult(1).replaceUsesWithIf(
  //     newCnot->getResult(1), [&](OpOperand &use) {
  //       return use.getOwner() !=
  //              rx; // only downstream/final uses, not the old internal edge
  //     });
}
