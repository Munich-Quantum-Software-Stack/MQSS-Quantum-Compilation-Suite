/* This code and any associated documentation is provided "as is"

Copyright 2025 Munich Quantum Software Stack Project

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
-------------------------------------------------------------------------
  author Martin Letras
  date   April 2025
  version 1.0
  brief
    Definition of the optimization levels supported by the MQSS.

*******************************************************************************
* This source code and the accompanying materials are made available under    *
* the terms of the Apache License 2.0 which accompanies this distribution.    *
******************************************************************************/

#include "Optimizer/Pipelines.hpp"
#include "Passes/Transforms.hpp"

#include <mlir/Transforms/Passes.h>

using namespace mlir;

void mqss::opt::O1(PassManager &pm) {
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createCommuteCxRxPass());
  // more passes to be added here
}

void mqss::opt::O2(PassManager &pm) {
  pm.addPass(createCanonicalizerPass());
  // more passes to be added here
}

void mqss::opt::O3(PassManager &pm) {
  // more passes to be added here
  pm.addPass(createCancellationDoubleCxPass());
}
