/****************************************************************-*- C++ -*-****
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#ifdef BUILD_CUDAQ_ENABLED
#include "../../cudaq/include/Utils/utils.h"
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "../../catalyst/include/Utils/utils.h"
#endif

using namespace llvm;
using namespace mlir;

struct DialectInfo {

  SmallVector<Operation *, 16> QuantumKernels;
  std::vector<Operation *> GateOps;
};

struct MyModuleAnalysis {

public:
  MyModuleAnalysis(mlir::ModuleOp module);


// #ifdef BUILD_CUDAQ_ENABLED
  std::vector<Operation *> getGateOps();

  void fetchQuantumKernels();

  DialectInfo getDialectInfo(){
    return Info;
  }
//#endif

private:
  void gatherOperations();
  void initialize(mlir::ModuleOp module);

  mlir::ModuleOp module;
  DialectInfo Info;

};

extern std::unique_ptr<MyModuleAnalysis> analysis;

struct MyModuleAnalysisPass
    : public mlir::PassWrapper<MyModuleAnalysisPass,
                               mlir::OperationPass<mlir::ModuleOp>> {

  void runOnOperation() override {
    auto module = getOperation();
    analysis = std::make_unique<MyModuleAnalysis>(module);
  }
};