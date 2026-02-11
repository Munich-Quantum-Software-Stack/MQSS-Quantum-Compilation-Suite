/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "Decompositions.hpp"
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
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect>();

  // 3. Keep your custom dialects
  registry.insert<cudaq::cc::CCDialect, quake::QuakeDialect>();

  // 2. Register ONLY the passes you need
    mlir::registerCSEPass();           // Enables --cse
    mlir::registerCanonicalizerPass(); // Enables --canonicalize

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

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCancellationDoubleCxPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCustomExamplePass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createHXHToZPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createHZHToXPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCancellationNullRotationPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createNormalizeArgAnglePass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createQuakeToLinAlgPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createReductionPatternPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCxToHCzHDecompositionPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createCzToHCxHDecompositionPass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createReverseCxPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSAdjZToSPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createSZToSAdjPass();
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createPrintQuakeGatesPass(llvm::outs());
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    static std::string configData = "allow_cx=true; allow_rz=false"; 
    
    // 2. Create the Stream
    // We make it static so it survives after the lambda returns 
    // (important if the pass holds a reference to it!)
    static std::istringstream inputSource(configData);
    
    // 3. Reset the stream (Critical!)
    // If you run this pass multiple times, the stream cursor will be at the end.
    // We must rewind it to the beginning every time the pass is created.
    inputSource.clear();
    inputSource.seekg(0);
    return mqss::opt::createQASM3ToQuakePass(inputSource);
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::opt::createQuakeToTikzPass(llvm::outs());
  });

  

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}