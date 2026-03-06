/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "Extractor.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#ifdef BUILD_CUDAQ_ENABLED
#include "../../mqss-cudaq/include/Utils/utils.h"
#include "Utils/Quake.hpp"
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "../../mqss-catalyst/include/Utils/utils.h"
#endif

using namespace llvm;
using namespace mlir;

// Module Initialization
void MyModuleAnalysis::initialize(mlir::ModuleOp module) {
  MyModuleAnalysis::module = module;
}

void MyModuleAnalysis::fetchQuantumKernels() {

  auto walkResult = module.walk([&](Operation *op) {
    // Check if it is a quantum kernel
    // TODO (Akshay): Check here for catalyst kernel?
    if (auto funcOp = dyn_cast<func::FuncOp>(op)) {
#ifdef BUILD_CUDAQ_ENABLED
      if (funcOp->hasAttr(cudaq::entryPointAttrName)) {
        Info.QuantumKernels.push_back(funcOp);
        return WalkResult::advance();
      }
      for (auto arg : funcOp.getArguments()) {
        if (isa<quake::RefType, quake::VeqType>(arg.getType())) {
          Info.QuantumKernels.push_back(funcOp);
          return WalkResult::advance();
        }
      }
#endif
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

#ifdef BUILD_CUDAQ_ENABLED
    // Check if it is controlled quake.apply
    if (auto applyOp = dyn_cast<quake::ApplyOp>(op))
      if (!applyOp.getControls().empty())
        return WalkResult::interrupt();
#endif
    return WalkResult::advance();
  });

  //   return std::make_tuple(Info.QuantumKernels, walkResult);
}

// Constructor
MyModuleAnalysis::MyModuleAnalysis(mlir::ModuleOp module) {
  initialize(module);
  fetchQuantumKernels();
  gatherOpInfo();
}

// Gather Information (QuantumView) about every operation
void MyModuleAnalysis::gatherOpInfo() {
  for (auto kernel : Info.QuantumKernels) {

    kernel.getBody().walk([&](Operation *Op) {
      QuantumOpView OpView;
      if (!mlir::isMemoryEffectFree(Op))
        OpView.hasSideEffects = true;

      if (Op->getDialect()->getNamespace() == "quake") {
#ifdef BUILD_CUDAQ_ENABLED
        if (auto gate = dyn_cast<quake::OperatorInterface>(Op)) {
          // OpView.InputQubits = gate.getControls();
          for (auto t : gate.getControls()) {
            if (auto ext_ref =
                    dyn_cast<quake::ExtractRefOp>(t.getDefiningOp())) {
              QubitID ID;
              Value base = ext_ref.getVeq();
              auto index = ext_ref.getConstantIndex();
              ID.base = base;
              ID.index = index;
              OpView.InputQubits.push_back(ID);
            }
          }
          for (auto t : gate.getTargets()) {
            if (auto ext_ref =
                    dyn_cast<quake::ExtractRefOp>(t.getDefiningOp())) {
              QubitID ID;
              Value base = ext_ref.getVeq();
              auto index = ext_ref.getConstantIndex();
              ID.base = base;
              ID.index = index;
              OpView.InputQubits.push_back(ID);
            }
          }

          auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
          if (isQGateOp)
            OpView.GateTy = GateTy;

          OpView.OutputQubits = OpView.InputQubits;
        }

#endif
      }
      if (Op->getDialect()->getNamespace() == "quantum") {
#ifdef BUILD_CATALYST_ENABLED
        // Only consider operations on gates with side-effects
        if (auto g = isCatalystQuantumGateOp(Op)) {
          if (auto mem = dyn_cast<MemoryEffectOpInterface>(Op))
            if (!mem.hasNoEffect())
              OpView.hasSideEffects = true;
          for (unsigned i = 0; i < g.getNumOperands(); i++) {
            QubitID ID;
            ID.base = g->getOperand(i);
            ID.index = -1;
            OpView.InputQubits.push_back(ID);
          }

          for (unsigned i = 0; i < g->getNumResults(); i++) {
            QubitID ID;
            ID.base = g->getResult(i);
            ID.index = -1;
            OpView.OutputQubits.push_back(ID);
          }
          OpView.GateTy = g.getGateName();
        }
#endif
      }
      Info.OpQuantumView[Op] = OpView;
    });
  }
}

bool MyModuleAnalysis::touchesAny(Operation *Op2,
                                  std::vector<QubitID> Op1QubitIDs) {


  for (auto op : Op2->getOperands()) {
    // Catalyst case
    for (auto Op1Qubit : Op1QubitIDs) {
      if (Op1Qubit.index == -1) {
        if (op == Op1Qubit.base)
          return true;                // Return if an operand Qubit of Op1 is used in Op2
      }

// Quake case
#ifdef BUILD_CUDAQ_ENABLED
      if (auto ext = dyn_cast<quake::ExtractRefOp>(op.getDefiningOp())) {
        if (ext.getVeq() == Op1Qubit.base &&
            ext.getConstantIndex() == Op1Qubit.index)
        return true;                    // Return if an operand Qubit of Op1 is used in Op2
      }
#endif
    }
  }

  return false;
}

bool equivalence_check(const std::vector<QubitID> &Op1,
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

bool MyModuleAnalysis::sameQubits(tupleVectorsQubitIDs Op1InOuts,
                                  tupleVectorsQubitIDs Op2InOuts) {

  auto &[Op1Inputs, Op1Outputs] = Op1InOuts;
  auto &[Op2Inputs, Op2Outputs] = Op2InOuts;

  auto Inputcheck = equivalence_check(Op1Inputs, Op2Inputs);
  if (!Inputcheck)
    return false;
  auto OutputCheck = equivalence_check(Op1Outputs, Op2Outputs);

  return true;
}

std::vector<Operation *> MyModuleAnalysis::getGateOps() {

  if (Info.QuantumKernels.empty()) {
    llvm::outs() << "Empty kernels\n";
    return {};
  }

  for (auto kernel : Info.QuantumKernels) {
    kernel->walk([&](Operation *Op) {
      if (Op->getDialect()->getNamespace() == "quake") {
#ifdef BUILD_CUDAQ_ENABLED
        auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
        if (isQGateOp)
          Info.GateOps.push_back(Op);
#endif
      }

      if (Op->getDialect()->getNamespace() == "quantum") {
#ifdef BUILD_CATALYST_ENABLED
        if (isCatalystQuantumGateOp(Op))
          Info.GateOps.push_back(Op);
#endif
      }
    });
  }
  return Info.GateOps;
}
