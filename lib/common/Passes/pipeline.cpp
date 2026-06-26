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

#include "include/transforms/PassUtils.h"

using namespace mlir;

void mqss_backend::O1(mlir::OpPassManager &pm) {

  CommonMappingPassOptions MappingOpts;
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