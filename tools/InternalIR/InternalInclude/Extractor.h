/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/


#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/raw_ostream.h"

#ifdef BUILD_CUDAQ_ENABLED
#include "../../cudaq/include/Utils/utils.h"
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "../../catalyst/include/Utils/utils.h"
#endif

using namespace llvm;
using namespace mlir;


struct MyModuleAnalysis {

public:
  MyModuleAnalysis(mlir::ModuleOp module) : module(module) {}

  SmallVector<mlir::func::FuncOp> getModuleFunctions() {
    SmallVector<mlir::func::FuncOp> functions;
    module.walk([&](mlir::func::FuncOp f) { functions.push_back(f); });

    return functions;
  }

  std::vector<Operation *> gatherOperations() {

    module->walk([&](Operation *op) { funcOperations.push_back(op); });

    return funcOperations;
  }

  #ifdef BUILD_CATALYST_ENABLED
  std::vector<Operation *> getCatalystGateOps() {
    if (funcOperations.empty())
      funcOperations = gatherOperations();

    for (auto *Op : funcOperations) {
      if (Op->getDialect()->getNamespace() == "quantum") {
            catalystquantumGateOps.push_back(Op);
      }
    }

    return catalystquantumGateOps;
  }
  #endif

  #ifdef BUILD_CUDAQ_ENABLED
  std::vector<Operation *> getQuakeGateOps() {
    if (funcOperations.empty())
      funcOperations = gatherOperations();

    for (auto *Op : funcOperations) {
      if (Op->getDialect()->getNamespace() == "quake") {
        if (isQuakeGateOp(Op)) {
            quakeGateOps.push_back(Op);
        }
      }
    }

    return quakeGateOps;
  }
  #endif

private:
  mlir::ModuleOp module;
  std::vector<Operation *> funcOperations;
  std::vector<Operation *> quakeGateOps;
  std::vector<Operation *> catalystquantumGateOps;
};

struct MyModuleAnalysisPass
    : public mlir::PassWrapper<MyModuleAnalysisPass,
                               mlir::OperationPass<mlir::ModuleOp>> {

  void runOnOperation() override {
    auto module = getOperation();
    analysis = std::make_unique<MyModuleAnalysis>(module);
  }

  std::unique_ptr<MyModuleAnalysis> analysis;
};