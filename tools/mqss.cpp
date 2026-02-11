/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "Examples.hpp"
#include "Optimizer/Pipelines.hpp"
#include "Passes/CodeGen.hpp"
#include "Transforms.hpp"
#include "common/RuntimeMLIR.h"
#include "cudaq/Optimizer/Dialect/CC/CCDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "mlir/IR/Dialect.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassRegistry.h>


int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // ... your dialect setup ...
    
    // 2. Explicitly register the standard dialects 
    // registerAllDialects(registry) often fails due to linker stripping symbols
    registry.insert<mlir::func::FuncDialect, 
                    mlir::arith::ArithDialect, 
                    mlir::cf::ControlFlowDialect>();
    
    // 3. Keep your custom dialects
    registry.insert<cudaq::cc::CCDialect, quake::QuakeDialect>();

  // 1. Register the Pipelines (These show up under "Pass Pipelines")
  // mlir::PassPipelineRegistration<>("mqss-o1", "MQSS O1", mqss::opt::O1);
  mlir::PassPipelineRegistration<>(
      "mqss-o1", "Run MQSS O1", [](mlir::OpPassManager &pm) {
        // We have to cast because MQSS expects the 'Boss' (PassManager)
        // This works because PassManager inherits from OpPassManager
        mqss::opt::O1(static_cast<mlir::PassManager &>(pm));
      });

  mlir::PassPipelineRegistration<>(
      "mqss-o2", "Run MQSS O2", [](mlir::OpPassManager &pm) {
        // We have to cast because MQSS expects the 'Boss' (PassManager)
        // This works because PassManager inherits from OpPassManager
        mqss::opt::O2(static_cast<mlir::PassManager &>(pm));
      });

  mlir::PassPipelineRegistration<>(
      "mqss-o3", "Run MQSS O3", [](mlir::OpPassManager &pm) {
        // We have to cast because MQSS expects the 'Boss' (PassManager)
        // This works because PassManager inherits from OpPassManager
        mqss::opt::O3(static_cast<mlir::PassManager &>(pm));
      });

  // 2. Register Individual Passes (This makes them show up under "Passes")
  // Use the explicit registration call:
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteCxRxPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteZCxPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteCxZPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteCxXPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteRxCxPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCommuteXCxPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchPauliHPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchHXPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchHZPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchXHPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchZHPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchHYPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSwitchYHPass();
  });

  std::string inputfile = argv[1];

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createPrintQuakeGatesPass(llvm::outs());;
  });

  //    mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
  //      return mqss::opt::createCancellationDoubleCxPass();
  //    });

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}