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
*************************************************************************
  author Akshay Bhosale
  co-author: Claude AI Sonnet/Opus
  date   August 2026
  version 2.0.0
*************************************************************************/

#include "cudaq/Optimizer/CodeGen/Passes.h"
#include "Passes/Transforms/PassUtils.h"
#include "Passes/Transforms/Transforms.h"
#include <llvm/ADT/StringRef.h>
#include <string>

using namespace mlir;

static void addQIRConversionPipeline(mlir::OpPassManager &pm,
                                     llvm::StringRef convertTo) {
  auto convertFields = convertTo.split(':');
  if (convertFields.first == "qir" || convertFields.first == "qir-full") {
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "full:" +
                                                   convertFields.second.str());
  } else if (convertFields.first == "qir-base") {
    pm.addNestedPass<mlir::func::FuncOp>(cudaq::opt::createDelayMeasurements());
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "base-profile:" +
                                                   convertFields.second.str());
  } else if (convertFields.first == "qir-adaptive") {
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "adaptive-profile:" +
                                                   convertFields.second.str());
  } else {
    
  }
}

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

  pm.addPass(createCommonGateCancellationPass(CancelOpts));
  pm.addPass(CommonCNOTReversePass());
  pm.addPass(createCommonCommutePass(CommuteOpts));

  // Standard MLIR Passes
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O3(mlir::OpPassManager &pm) {

  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::QIRConversionPipeline(mlir::OpPassManager &pm, StringRef converTo) {

  //cudaq::opt::ReturnToOutputLogOptions opts;
  // auto tgt = StringRef(converTo).split(':').first;
  // opts.allowDynamicResult = tgt == "qir" || tgt == "qir-full";
  addQIRConversionPipeline(pm, converTo);
  //pm.addPass(cudaq::opt::createReturnToOutputLog(opts));
  // pm.addPass(createConvertMathToFuncs());
  pm.addPass(createSymbolDCEPass());
  pm.addPass(cudaq::opt::createCCToLLVM());
  pm.addPass(LLVMDialectToLLVMIRPass());
}