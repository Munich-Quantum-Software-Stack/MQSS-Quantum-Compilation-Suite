/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace mlir;

enum Gate { CNOT, PAULIX, H, RX, Z, Y, NA };

inline Gate parseGateTy(const StringRef &GateTy) {
  if (GateTy == "CNOT")
    return CNOT;
   if (GateTy == "PAULIX")
    return PAULIX;
  if (GateTy == "H")
    return H;
  if (GateTy == "RX")
    return RX;
  if (GateTy == "Z")
    return Z;
  if (GateTy == "Y")
    return Y;
  return NA;
}

struct QubitID {
  Value base;
  std::size_t index;
};

// TODO (Akshay): Is it better to have separate ControlQubits
//                and TargetQubits vectors?
struct QuantumOpView {
  Gate GateTy=Gate::NA;                   //<---------- Important to initialize enums
  std::vector<QubitID> ControlQubits = {};
  std::vector<QubitID> TargetQubits = {};
  bool hasSideEffects = false;
};

struct DialectInfo {

  SmallVector<func::FuncOp, 16> QuantumKernels;
  std::vector<Operation *> GateOps;
  std::unordered_map<Operation *, QuantumOpView> OpQuantumView;
};

using tupleVectorsQubitIDs =
    std::tuple<std::vector<QubitID>, std::vector<QubitID>>;

inline bool equivalence_check(const std::vector<QubitID> &Op1,
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

class MyModuleAnalysis {

public:
  virtual ~MyModuleAnalysis() = default;
  virtual std::vector<Operation *> getGateOps()=0; // Pure virtual functions that need to be implemented by
                    // derived classes

  virtual void fetchQuantumKernels()=0;

  virtual void gatherOpInfo()=0;

  virtual bool touchesAny(Operation *Op2, std::vector<QubitID> Op1QubitIDs)=0;

  virtual mlir::LogicalResult verifyModule()=0;

  DialectInfo getDialectInfo(){
    return Info;
  }

  bool sameQubits(tupleVectorsQubitIDs Op1CtrlTarget,
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


protected:
  DialectInfo Info;

};