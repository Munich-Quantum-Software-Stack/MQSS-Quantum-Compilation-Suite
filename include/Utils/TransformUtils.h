

#include "Passes/Analysis/DialectAnalysisSelector.h"
#include "Quantum/IR/QuantumOps.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

#include <mlir/IR/Attributes.h>
#include <mlir/IR/BuiltinTypeInterfaces.h>
#include <mlir/IR/Types.h>
#include <mlir/IR/Value.h>
#include <vector>

using namespace mlir;
using namespace llvm;
using namespace cudaq;
using namespace catalyst;

#include <numbers>

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

static std::vector<mlir::Value>
getQubitValues(std::vector<QubitID> QubitVector) {
  std::vector<mlir::Value> QubitValues;
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

static mlir::Value normalizeValue(double param, mlir::IRRewriter &builder,
                                  Location loc) {

  double pi = std::numbers::pi;
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

static bool touchesAny(Operation *Op2,
                       SmallVector<mlir::Value, 2> Op1OutQubits) {

  for (auto OutQubit : Op1OutQubits) {

    for (auto Op2op : Op2->getOperands()) {
      if ((Op2op == OutQubit)) {
        return true;
      }
    }
  }
  return false;
}

inline quake::OperatorInterface
createQuakeGate(Location loc, llvm::StringRef NewGateTy,
                const SmallVector<mlir::Value, 2> ControlQubits,
                const SmallVector<mlir::Value, 2> TargetQubitOps,
                const mlir::ValueRange params, mlir::IRRewriter &builder,
                bool isAdj = false) {

  if (NewGateTy == "RX") {
    // TODO: Can this create an gate?
    return builder.create<quake::RxOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RY") {
    // TODO: Can this create an gate?
    return builder.create<quake::RyOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RZ") {
    // TODO: Can this create an gate?
    return builder.create<quake::RzOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "S") {
    return builder.create<quake::SOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "T") {
    return builder.create<quake::TOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "SAdj") {
    return builder.create<quake::SOp>(loc, true, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "TAdj") {
    return builder.create<quake::TOp>(loc, true, params, ControlQubits,
                                      TargetQubitOps);
  }

  if (NewGateTy == "PauliZ") {
    return builder.create<quake::ZOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliX") {
    return builder.create<quake::XOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliY") {
    return builder.create<quake::YOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "H") {
    return builder.create<quake::HOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "CNOT" || NewGateTy == "CX") {
    return builder.create<quake::XOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CY") {
    return builder.create<quake::YOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CZ") {
    return builder.create<quake::ZOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "SWAP") {
    return builder.create<quake::SwapOp>(loc, params, ControlQubits,
                                         TargetQubitOps);
  }
  return nullptr;
}

inline mlir::arith::ConstantOp
createConstant(Location loc, mlir::IRRewriter &builder, TypedAttr attr) {

  // arith::ConstantOp with FloatAttr works on both LLVM-16 and LLVM-21
  return builder.create<mlir::arith::ConstantOp>(loc, attr);
}

inline mlir::arith::ConstantOp createQuakeConstOp(Location loc,
                                                  mlir::IRRewriter &builder,
                                                  double constantValue,
                                                  mlir::Type type) {

  if (isa<FloatType>(type)) {
    auto valueAttr = builder.getFloatAttr(type, constantValue);
    return createConstant(loc, builder, valueAttr);
  }
}

inline mlir::Value createQuakeDivF(Location loc, mlir::Value numerator,
                                   double denominator,
                                   mlir::IRRewriter &rewriter) {
  auto denominatorValue = rewriter.create<arith::ConstantOp>(
      loc, rewriter.getF64FloatAttr(denominator));
  return rewriter.create<arith::DivFOp>(loc, numerator, denominatorValue);
}

inline quake::AllocaOp
createQuakeAlloca(Location loc, mlir::IRRewriter &builder, size_t numQubits) {

  return builder.create<quake::AllocaOp>(
      loc, quake::VeqType::get(builder.getContext(), numQubits));
}

inline quake::ExtractRefOp createQuakeExtractRefOp(Location loc,
                                                   mlir::IRRewriter &builder,
                                                   mlir::Value qubits,
                                                   unsigned int targetQubit) {

  return builder.create<quake::ExtractRefOp>(loc, qubits, targetQubit);
}

inline SmallVector<mlir::Value, 2>
createQuakeMeasureOp(Location loc, mlir::IRRewriter &builder,
                     const SmallVector<mlir::Value, 2> TargetQubits) {

  SmallVector<mlir::Value, 2> results;
  mlir::Type measTy = quake::MeasureType::get(builder.getContext());
  auto newOp = builder.create<quake::MzOp>(loc, measTy, TargetQubits);

  for (auto res : newOp->getResults()) {
    results.push_back(res);
  }

  return results;
}

inline quantum::CustomOp
createCatalystGate(Location loc, llvm::StringRef NewGateTy,
                   const SmallVector<mlir::Value, 2> ControlQubitsOps,
                   const SmallVector<mlir::Value, 2> TargetQubitOps,
                   const mlir::ValueRange params, mlir::IRRewriter &builder,
                   bool isAdj = false) {

  quantum::CustomOp NewOp = nullptr;
  if (ControlQubitsOps.empty()) {
    std::vector<mlir::Type> TargetQubitTys;

    for (auto t : TargetQubitOps) {
      TargetQubitTys.push_back(t.getType());
    }
    if (NewGateTy == "SAdj") {
      NewGateTy = "S";
      isAdj = true;
    }
    if (NewGateTy == "TAdj") {
      NewGateTy = "T";
      isAdj = true;
    }

    return builder.create<quantum::CustomOp>(
        loc,
        /*out_qubits=*/mlir::TypeRange(TargetQubitTys),
        /*out_ctrl_qubits=*/mlir::TypeRange(),
        /*params=*/params,
        /*in_qubits=*/mlir::ValueRange(TargetQubitOps),
        /*gate_name=*/NewGateTy,
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange(),
        /*in_ctrl_values=*/mlir::ValueRange());
    // TODO: Revisit this
    // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  }

  StringAttr GateTy;
  if (NewGateTy == "CNOT") {
    GateTy = builder.getStringAttr("CNOT");
  }
  if (NewGateTy == "CZ") {
    GateTy = builder.getStringAttr("CZ");
  }

  mlir::Value control = ControlQubitsOps[0];
  mlir::Value target = TargetQubitOps[0];

  return builder.create<quantum::CustomOp>(
      loc,
      /*out_qubits=*/mlir::TypeRange{control.getType(), target.getType()},
      /*out_ctrl_qubits=*/mlir::TypeRange{},
      /*params=*/mlir::ValueRange{},
      /*in_qubits=*/mlir::ValueRange{control, target},
      /*gate_name=*/GateTy, // or gateName if API
                            // accepts StringRef
      /*adjoint=*/isAdj,
      /*in_ctrl_qubits=*/mlir::ValueRange{},
      /*in_ctrl_values=*/mlir::ValueRange{});
  // TODO: Revisit this
  // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  // OpToReplace->getResult(1).replaceAllUsesWith(NewOp->getResult(1));
}

inline mlir::arith::ConstantOp createCatalystConstOp(Location loc,
                                                     mlir::IRRewriter &builder,
                                                     double constantValue,
                                                     mlir::Type type) {

  if (isa<FloatType>(type)) {
    auto valueAttr = builder.getFloatAttr(type, constantValue);
    return createConstant(loc, builder, valueAttr);
  }
}

inline mlir::Value createCatalystDivF(Location loc, mlir::Value numerator,
                                      double denominator,
                                      mlir::IRRewriter &rewriter) {
  auto denominatorValue = rewriter.create<arith::ConstantOp>(
      loc, rewriter.getF64FloatAttr(denominator));
  return rewriter.create<arith::DivFOp>(loc, numerator, denominatorValue);
}

inline quantum::AllocOp createCatalystAlloca(Location loc,
                                             mlir::IRRewriter &builder,
                                             size_t numQubits) {

  auto regTy = quantum::ResultType::get(builder.getContext());
  mlir::Type qregTy = mlir::parseType("!quantum.reg", builder.getContext());
  assert(qregTy && "failed to parse !quantum.reg");

  return builder.create<quantum::AllocOp>(
      loc, qregTy,
      /*nqubits=*/mlir::Value{},
      /*nqubits_attr=*/builder.getI64IntegerAttr(numQubits));
}

inline quantum::ExtractOp createCatalystExtractRefOp(Location loc,
                                                     mlir::IRRewriter &builder,
                                                     mlir::Value allocQubits,
                                                     unsigned int targetQubit) {

  mlir::Type qbitTy = mlir::parseType("!quantum.bit", builder.getContext());

  assert(qbitTy && "failed to parse !quantum.bit");

  auto indexAttr = builder.getI64IntegerAttr(targetQubit);

  return builder.create<quantum::ExtractOp>(loc, qbitTy, allocQubits,
                                            /*index=*/mlir::Value{},
                                            /*index_attr=*/indexAttr);
}

inline SmallVector<mlir::Value, 2>
createCatalystMeasureOp(mlir::Location loc, mlir::IRRewriter &builder,
                        const SmallVector<mlir::Value, 2> TargetQubitOps) {

  SmallVector<mlir::Value> results;
  auto i1Ty = builder.getI1Type();
  for (mlir::Value q : TargetQubitOps) {
    auto qbitTy = q.getType(); // should be !quantum.bit
    auto m =
        builder.create<quantum::MeasureOp>(loc, TypeRange{i1Ty, qbitTy}, q);
    // measured classical result
    auto measResult = m.getMres();
    // updated SSA qubit
    auto outQubit = m.getOutQubit();

    results.push_back(measResult);
    results.push_back(outQubit);
  }
  // // Access results
  // mlir::Value outQubit = measureOp.getOutQubit(); // !quantum.bit
  // mlir::Value mres     = measureOp.getMres();     // i1
  return results;
}

static void eraseOpsSafely(llvm::SmallPtrSetImpl<mlir::Operation *> &eraseSet) {
  llvm::SmallVector<mlir::Operation *> ordered;

  llvm::SmallVector<mlir::Value> operandsToCleanup;

  for (mlir::Operation *op : eraseSet) {
    ordered.push_back(op);
  }

  // Post-order walk usually gives users before producers if rooted properly,
  // but safest simple approach: repeatedly erase ops with no remaining users
  // outside the erase set.
  bool changed = true;

  while (!ordered.empty() && changed) {
    changed = false;

    for (auto it = ordered.begin(); it != ordered.end();) {
      mlir::Operation *op = *it;
      llvm::outs() << "Controlled DCE for: " << *op << "\n";

      bool hasInternalUsersLeft = false;
      for (mlir::Value result : op->getResults()) {
        for (mlir::Operation *user : result.getUsers()) {
          if (eraseSet.contains(user)) {
            hasInternalUsersLeft = true;
            break;
          }
        }
        if (hasInternalUsersLeft)
          break;
      }

      if (!hasInternalUsersLeft) {
        op->erase();
        it = ordered.erase(it);
        changed = true;
      } else {
        ++it;
      }
    }
  }

  // cleanupDeadDefs(operandsToCleanup);

  assert(ordered.empty() && "cycle or invalid erase dependency");
}

static Operation *createNewGate(Location loc, QuantumDialect DialectTy,
                                llvm::StringRef NewGateTy,
                                SmallVector<mlir::Value, 2> ControlQubitOps,
                                SmallVector<mlir::Value, 2> TargetQubitOps,
                                const mlir::ValueRange params,
                                mlir::IRRewriter &builder, bool isAdj = false) {

  Operation *NewOp;

  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create Op for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp = createQuakeGate(loc, NewGateTy, ControlQubitOps, TargetQubitOps,
                            params, builder, isAdj);
  } else {
    NewOp = createCatalystGate(loc, NewGateTy, ControlQubitOps, TargetQubitOps,
                               params, builder, isAdj);
  }

  return NewOp;
}

static mlir::Value createAllocOp(Location loc, QuantumDialect DialectTy,
                                 mlir::IRRewriter &builder, size_t numQubits) {

  mlir::Value NewOp;
  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create AllocOp for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp = createQuakeAlloca(loc, builder, numQubits);
  } else {
    NewOp = createCatalystAlloca(loc, builder, numQubits);
  }

  return NewOp;
}

static mlir::Value createExtractOp(Location loc, QuantumDialect DialectTy,
                                   mlir::IRRewriter &builder,
                                   mlir::Value qubits,
                                   unsigned int targetQubit) {

  mlir::Value NewOp;
  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create ExtractRefOp for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp = createQuakeExtractRefOp(loc, builder, qubits, targetQubit);
  } else {
    NewOp = createCatalystExtractRefOp(loc, builder, qubits, targetQubit);
  }
  return NewOp;
}

static mlir::Value createArithConstantOp(Location loc, QuantumDialect DialectTy,
                                         mlir::IRRewriter &builder,
                                         double constantValue) {

  mlir::Value NewOp;
  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create ArithConstantOp for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp =
        createQuakeConstOp(loc, builder, constantValue, builder.getF64Type());
  } else {
    NewOp = createCatalystConstOp(loc, builder, constantValue,
                                  builder.getF64Type());
  }
  return NewOp;
}

static mlir::Value createDivFOp(Location loc, QuantumDialect DialectTy,
                                mlir::Value numerator, double denominator,
                                mlir::IRRewriter &rewriter) {

  mlir::Value NewOp;
  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create DivFOp for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp = createQuakeDivF(loc, numerator, denominator, rewriter);
  } else {
    NewOp = createCatalystDivF(loc, numerator, denominator, rewriter);
  }
  return NewOp;
}

static SmallVector<mlir::Value, 2>
createMeasureOp(Location loc, QuantumDialect DialectTy,
                mlir::IRRewriter &builder,
                const SmallVector<mlir::Value, 2> TargetQubits) {

  SmallVector<mlir::Value, 2> NewOp;
  assert(DialectTy != QuantumDialect::Unknown &&
         "Cannot create DivFOp for Unknown Dialect Type");
  if (DialectTy == QuantumDialect::Quake) {
    NewOp = createQuakeMeasureOp(loc, builder, TargetQubits);
  } else {
    NewOp = createCatalystMeasureOp(loc, builder, TargetQubits);
  }
  return NewOp;
}
