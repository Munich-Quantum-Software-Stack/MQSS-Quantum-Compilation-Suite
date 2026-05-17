

#ifdef BUILD_CUDAQ_ENABLED
#include "MQSSQuakePasses/Pipelines.h"
#include "MQSSQuakePasses/Transforms.hpp"
#include "SemanticExtractLayer/QuakeExtractor.h"
namespace mqss_backend = mqss_cudaq::opt;

#define DialectAnalysis QuakeAnalysis

namespace mqss_cudaq::opt {
#define GEN_PASS_CLASSES
#include "MQSSQuakePasses/Transforms.h.inc"

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
                       MapVector<Operation *, QuantumOpView> OpQuantumView) {

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

static bool touchesAny(Operation *Op2, SmallVector<mlir::Value, 2> Op1OutQubits) {

  for (auto OutQubit : Op1OutQubits) {

    for (auto Op2op : Op2->getOperands()) {
      if ((Op2op == OutQubit)) {
        return true;
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


static std::optional<QubitID> getOriginQubit(mlir::Value v) {
  // llvm::outs() << "Checking: " << v << "\n";
  while (true) {
    auto definingOp = v.getDefiningOp();
    
    if (!definingOp){
      // llvm::outs().indent(4) << "def op is NULL\n";
      return std::nullopt; // block argument or unknown source
    }

    // llvm::outs().indent(4) << "def op: " << *definingOp << "\n";
    // Case 1: %q4 = quantum.extract %reg[4]
    if (definingOp->getName().getStringRef() == "quantum.extract") {
      mlir::Value reg = definingOp->getOperand(0);

      // You need to adapt this depending on how Catalyst stores the index.
      // Often it is an IntegerAttr.
      
      auto indexAttr = definingOp->getAttrOfType<mlir::IntegerAttr>("idx_attr");
      if (!indexAttr){
        // llvm::outs().indent(4) << "Index Attr is null\n";
        return std::nullopt;
      }
      // llvm::outs().indent(4) << "reg: " << reg << ", Index attr: " << indexAttr.getInt() << "\n";

      return QubitID{reg, indexAttr.getInt()};
    }

    // Case 2:
    // %out:2 = quantum.custom "CNOT"() %a, %b
    // %out#0 maps to operand 0
    // %out#1 maps to operand 1
    if (definingOp->getName().getStringRef() == "quantum.custom") {
      auto result = llvm::dyn_cast<mlir::OpResult>(v);
      if (!result)
        return std::nullopt;

      unsigned resultNo = result.getResultNumber();

      if (resultNo >= definingOp->getNumOperands())
        return std::nullopt;

      v = definingOp->getOperand(resultNo);
      continue;
    }

    return std::nullopt;
  }
}

