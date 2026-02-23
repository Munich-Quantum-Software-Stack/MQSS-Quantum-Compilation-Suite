/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

// #include "Decompositions.hpp"
// #include "Examples.hpp"
// #include "Optimizer/Pipelines.hpp"
// #include "Passes/CodeGen.hpp"
// #include "Transforms.hpp"
// #include "Interfaces/Extractor.hpp"
#include "InternalInclude/Extractor.h"
#include "Passes/Examples.hpp"
#include "common/RuntimeMLIR.h"
#include "cudaq/Optimizer/Dialect/CC/CCDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
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

using namespace mlir;

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // ... your dialect setup ...

  // 2. Explicitly register the standard dialects
  // registerAllDialects(registry) often fails due to linker stripping symbols 
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect>();

  // 3. Keep your custom dialects
  registry.insert<cudaq::cc::CCDialect, quake::QuakeDialect>();

  // For Catalyst / StableHLO
  // mlir::DialectRegistry catRegistry;
  // catRegistry.insert<catalyst::quantum::QuantumDialect>();
  // mlir::MLIRContext catContext(catRegistry);

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return cudaq::opt::createPrintQuakeGatesPass(llvm::outs());;
  });

  llvm::outs() << "Dialects have been registered!\n";

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));

}