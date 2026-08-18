
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

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Utils/DebugUtils.h"
#include "Utils/Error.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

namespace mqss::codegen {
#define GEN_PASS_DEF_LLVMDIALECTTOLLVMIRPASS
#include "Passes/CodeGen/CodeGen.h.inc"
} // namespace mqss::codegen

using namespace mlir;
using namespace llvm;

namespace {

class LLVMDialectToLLVMIR
    : public mqss::codegen::impl::LLVMDialectToLLVMIRPassBase<
          LLVMDialectToLLVMIR> {

public:
  LLVMDialectToLLVMIR() = default;

  explicit LLVMDialectToLLVMIR(llvm::raw_ostream &os) : os(os) {}

  void runOnOperation() override {

    MQSS_DEBUG("\n[Applying Pass: LLVMDialectToLLVMIR]\n");
    auto context = &getContext();
    mlir::ModuleOp module = getOperation();

    MQSS_DEBUG("\nTranslated output:\n");

    // Convert the module to LLVM IR in a new LLVM IR context.
    llvm::LLVMContext llvmContext;
    auto llvmModule = translateModuleToLLVMIR(module, llvmContext);

    if (!llvmModule)
      MQSSemitFatalError(module->getLoc(), "Failed to emit LLVM IR");

    llvmModule->print(os, nullptr);
  }

private:
  llvm::raw_ostream &os = llvm::outs();
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::codegen::LLVMDialectToLLVMIRPass() {
  return std::make_unique<LLVMDialectToLLVMIR>();
}

std::unique_ptr<mlir::Pass>
mqss::codegen::LLVMDialectToLLVMIRPass(llvm::raw_ostream &os) {
  return std::make_unique<LLVMDialectToLLVMIR>(os);
}
