/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Transforms/PassUtils.h"
#include "Passes/Transforms/Transforms.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

using namespace mlir;

void mqss_backend::O1(mlir::OpPassManager &pm) {
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O2(mlir::OpPassManager &pm) {

  // MQSS MLIR Passes
  CommonGateCancellationPassOptions CancelOpts;
  CommonCommutePassOptions CommuteOpts;
  CancelOpts.mode = "CancelGate";
  CommuteOpts.mode = "CX-RX";

  pm.addPass(CommonGateCancellationPass(CancelOpts));
  pm.addPass(CommonCNOTReversePass());
  pm.addPass(CommonCommutePass(CommuteOpts));

  // Standard MLIR Passes
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O3(mlir::OpPassManager &pm) {

  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::QIRConversionPipeline(mlir::OpPassManager &pm,
                                         const std::string &convertTo,
                                         llvm::raw_ostream &os) {

  addQIRConversionPipeline(pm, convertTo);
  // pm.addPass(cudaq::opt::createReturnToOutputLog(opts));
  //  pm.addPass(createConvertMathToFuncs());
  pm.addPass(createSymbolDCEPass());
  pm.addPass(cudaq::opt::createCCToLLVM());
  pm.addPass(mqss::codegen::LLVMDialectToLLVMIRPass(os));
}
