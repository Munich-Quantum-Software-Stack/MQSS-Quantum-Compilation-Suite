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
-------------------------------------------------------------------------
  author Martin Letras
  date   December 2024
  version 1.0
  brief
  PrintCatalystGatesPass(llvm::raw_string_ostream ostream)
  Example MLIR pass that shows how to traverse a Quantum kernel written in
  MLIR/CatalystQuantum.
  The pass prints in ostream the type of each quantum gate and its operand(s)
  qubits.

*******************************************************************************
* This source code and the accompanying materials are made available under    *
* the terms of the Apache License 2.0 which accompanies this distribution.    *
******************************************************************************/

#include "SemanticExtractLayer/CatalystExtractor.h"
#include "MQSSCatalystPasses/Analysis.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {

class PrintCatalystGates
    : public PassWrapper<PrintCatalystGates, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(PrintCatalystGates)

  PrintCatalystGates(llvm::raw_ostream &ostream) : outputStream(ostream) {}

  llvm::StringRef getArgument() const override {
    return "print-catalyst-gates-pass";
  }
  llvm::StringRef getDescription() const override {
    return "Example pass that traverses a given mlir module, print its gates "
           "and a description of the operands of each gate";
  }

  void runOnOperation() override {

    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();

    llvm::outs() << "Dumping catalyst GateOps:\n";
    for (auto *op : analysis.getGateOps()) {
      op->dump();
    }

   if (failed(analysis.verifyModule())) {
        llvm::errs() << "[" << getArgument() << "]" << " : MLIR Module verification failed\n";
    }
  }

private:
  llvm::raw_ostream &outputStream; // Store the output stream
};

} // namespace

std::unique_ptr<mlir::Pass>
mqss_catalyst::opt::createPrintCatalystGatesPass(llvm::raw_ostream &ostream) {
  return std::make_unique<PrintCatalystGates>(ostream);
}
