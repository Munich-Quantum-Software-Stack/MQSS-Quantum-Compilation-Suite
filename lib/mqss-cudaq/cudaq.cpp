/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "IR/Dialect/CC/CCDialect.h"
#include "IR/Dialect/Quake/QuakeDialect.h"
#include "MQSSQuakePasses/Examples.h"
#include "MQSSQuakePasses/Pipelines.h"
#include "MQSSQuakePasses/Transforms.h"
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
  mlir::registerPass(
      []() -> std::unique_ptr<mlir::Pass> { return mlir::createCSEPass(); });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss_cudaq::opt::createPrintQuakeGatesPass(llvm::outs());
  });

  mlir::registerPassPipeline(
      "O1",                            // pipeline name (used on CLI too)
      "MQSS-O1 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        // build your pipeline here
        mqss_cudaq::opt::O1(pm); // populate pm instead of returning a pass
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );
  mlir::registerPassPipeline(
      "O2",                            // pipeline name (used on CLI too)
      "MQSS-O2 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        // build your pipeline here
        mqss_cudaq::opt::O2(pm); // populate pm instead of returning a pass
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );
  mlir::registerPassPipeline(
      "O3",                            // pipeline name (used on CLI too)
      "MQSS-O3 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        // build your pipeline here
        mqss_cudaq::opt::O3(pm); // populate pm instead of returning a pass
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}
