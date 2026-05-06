/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/



#include "IR/Dialect/CC/CCDialect.h"
#include "IR/Dialect/Quake/QuakeDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"

#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassRegistry.h>

#include "MQSSCUDAQPasses/Examples.hpp"
#include "MQSSCUDAQPasses/Transforms.hpp"

using namespace mlir;

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // ... your dialect setup ...

  // 2. Explicitly register the standard dialects
  // registerAllDialects(registry) often fails due to linker stripping symbols
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::LLVM::LLVMDialect>();

  // 3. Keep your custom dialects
  registry.insert<cudaq::cc::CCDialect, quake::QuakeDialect>();

  // For Catalyst / StableHLO
  // mlir::DialectRegistry catRegistry;
  // catRegistry.insert<catalyst::quantum::QuantumDialect>();
  // mlir::MLIRContext catContext(catRegistry);

  registerMQSSTransformsPasses();

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCanonicalizerPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCSEPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss_cudaq::opt::createPrintQuakeGatesPass(llvm::outs());
  });

  // Defining test architecture
  Architecture arch{};
  /*
      3
     / \
    4   2
    |   |
    0---1
  */
  const CouplingMap cm = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
                          {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
  arch.loadCouplingMap(5, cm);
  // Defining the settings of the mqt-mapper
  Configuration settings{};
  settings.heuristic = Heuristic::GateCountMaxDistance;
  settings.layering = Layering::DisjointQubits;
  settings.initialLayout = InitialLayout::Identity;
  settings.preMappingOptimizations = false;
  settings.postMappingOptimizations = false;
  settings.lookaheadHeuristic = LookaheadHeuristic::None;
  settings.debug = false;
  settings.addMeasurementsToMappedCircuit = true;

  mlir::registerPass([arch, settings]() mutable -> std::unique_ptr<mlir::Pass> {
    return mqss_cudaq::opt::createQuakeQMapPass(arch, settings);
  });

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}

