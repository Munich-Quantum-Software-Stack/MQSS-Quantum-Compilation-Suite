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
      if (Op->getDialect()->getNamespace() == "quake") {
#ifdef BUILD_CUDAQ_ENABLED
        //    Only Gate operations have Memory effects
        auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
        if (isQGateOp) {
          QuantumOpView OpView;
          OpView.GateTy = GateTy;
          auto gate = dyn_cast<quake::OperatorInterface>(Op);
          // OpView.InputQubits = gate.getControls();
          for (auto t : gate.getControls())
            OpView.InputQubits.push_back(t);
          for (auto t : gate.getTargets())
            OpView.InputQubits.push_back(t);

          OpView.OutputQubits = OpView.InputQubits;
          Info.OpQuantumView[Op] = OpView;
        }
#endif
      }
      if (Op->getDialect()->getNamespace() == "quantum") {
#ifdef BUILD_CATALYST_ENABLED
        // Only consider operations on gates with side-effects
        if (auto g = isCatalystQuantumGateOp(Op)) {
          QuantumOpView OpView;
          for (unsigned i = 0; i < g.getNumOperands(); i++) {
            OpView.InputQubits.push_back(g->getOperand(i));
          }

          for (unsigned i = 0; i < g->getNumResults(); i++) {
            OpView.OutputQubits.push_back(g->getResult(i));
          }
          OpView.GateTy = g.getGateName();
          Info.OpQuantumView[Op] = OpView;
        }
#endif
      }
    });
  }

  // for (auto &[Op, View] : Info.OpQuantumView) {
  //   llvm::outs() << "Op: " << *Op << "\n";
  //   llvm::outs().indent(4) << "gate: " << View.GateTy << "\n";
  //   for (auto op : View.InputQubits)
  //     llvm::outs().indent(4) << "input qubit: " << op << "\n";
  //   for (auto opRes : View.OutputQubits)
  //     llvm::outs().indent(4) << "Target qubit: " << opRes << "\n";
  // }
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
