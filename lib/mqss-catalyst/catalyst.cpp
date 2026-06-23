/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*************************************************************************
  author Akshay Bhosale
  date   February 2026
  version 1.0
*************************************************************************/

#include "IR/QuantumDialect.h"
#include "MQSSCatalystPasses/Analysis.h"
#include "MQSSCatalystPasses/Pipelines.h"
#include "MQSSCatalystPasses/Transforms.h"
#include "mlir/IR/Dialect.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassRegistry.h>

using namespace llvm;
int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // ... your dialect setup ...

  // 2. Explicitly register the standard dialects
  // registerAllDialects(registry) often fails due to linker stripping symbols
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::linalg::LinalgDialect,
                  mlir::scf::SCFDialect, mlir::LLVM::LLVMDialect>();

  // 3. Keep your custom dialects
  registry.insert<mlir::tensor::TensorDialect>();
  registry.insert<catalyst::quantum::QuantumDialect>();

  // For Catalyst / StableHLO (Maybe in the future)
  // mlir::DialectRegistry catRegistry;
  // catRegistry.insert<catalyst::quantum::QuantumDialect>();
  // mlir::MLIRContext catContext(catRegistry);

  registerMQSSTransformsPasses();

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss_catalyst::opt::createPrintCatalystGatesPass(llvm::outs());
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCanonicalizerPass();
  });
  mlir::registerPass(
      []() -> std::unique_ptr<mlir::Pass> { return mlir::createCSEPass(); });

  mlir::registerPassPipeline(
      "O1",                            // pipeline name (used on CLI too)
      "MQSS-O1 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        // build your pipeline here
        mqss_catalyst::opt::O1(pm); // populate pm instead of returning a pass
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
        mqss_catalyst::opt::O2(pm); // populate pm instead of returning a pass
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
        mqss_catalyst::opt::O3(pm); // populate pm instead of returning a pass
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}