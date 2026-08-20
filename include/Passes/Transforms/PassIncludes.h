
/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#include "Passes/Analysis/DialectAnalysisSelector.h"
#include "Pipelines.h"
#include "Transforms.h"

#include <mlir/IR/Value.h>
#include <vector>

#define DialectAnalysis QuakeAnalysis

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
