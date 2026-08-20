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
#include "Passes/CodeGen/Patterns.h"
#include "Quantum/IR/QuantumOps.h"
#include "Utils/Error.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

#include <mlir/Conversion/ArithToLLVM/ArithToLLVM.h>
#include <mlir/IR/BuiltinTypes.h>
#include <mlir/IR/Types.h>

using namespace mlir;
using namespace catalyst::quantum;

namespace catalyst {
namespace quantum {

#define GEN_PASS_DECL_QUANTUMCONVERSIONPASS
#define GEN_PASS_DEF_QUANTUMCONVERSIONPASS
#include "Passes/CodeGen/CodeGen.h.inc"

struct QIRTypeConverter : public LLVMTypeConverter {

  QIRTypeConverter(MLIRContext *ctx) : LLVMTypeConverter(ctx) {
    addConversion([&](QubitType type) { return convertQubitType(type); });
    addConversion([&](QuregType type) { return convertQuregType(type); });
    addConversion(
        [&](ObservableType type) { return convertObservableType(type); });
    addConversion([&](ResultType type) { return convertResultType(type); });
  }

private:
  LLVM::LLVMPointerType convertQubitType(mlir::Type mlirType) {
    return LLVM::LLVMPointerType::get(
        &getContext()); // LLVM::LLVMStructType::getOpaque("Qubit",
                        // &getContext());
  }

  LLVM::LLVMPointerType convertQuregType(mlir::Type mlirType) {
    return LLVM::LLVMPointerType::get(
        &getContext()); // LLVM::LLVMStructType::getOpaque("Array",
                        // &getContext());
  }

  mlir::Type convertObservableType(mlir::Type mlirType) {
    return this->convertType(mlir::IntegerType::get(&getContext(), 64));
  }

  LLVM::LLVMPointerType convertResultType(mlir::Type mlirType) {
    return LLVM::LLVMPointerType::get(
        &getContext()); // LLVM::LLVMStructType::getOpaque("Result",
                        // &getContext());
  }
};

struct QuantumConversion : impl::QuantumConversionPassBase<QuantumConversion> {
  using QuantumConversionPassBase::QuantumConversionPassBase;

  void runOnOperation() final {
    MLIRContext *context = &getContext();
    auto module = getOperation();
    QIRTypeConverter typeConverter(context);

    RewritePatternSet patterns(context);
    arith::populateArithToLLVMConversionPatterns(typeConverter, patterns);
    cf::populateControlFlowToLLVMConversionPatterns(typeConverter, patterns);
    populateFuncToLLVMConversionPatterns(typeConverter, patterns);
    cf::populateAssertToLLVMConversionPattern(typeConverter, patterns);

    populateQIRConversionPatterns(typeConverter, patterns,
                                  useArrayBackedRegisters);

    LLVMConversionTarget target(*context);
    target.addLegalOp<ModuleOp>();
    // target.addLegalOp<func::FuncOp>();   // keep func ops as-is

    if (failed(
            applyFullConversion(getOperation(), target, std::move(patterns)))) {
      MQSSemitFatalError(module->getLoc(), "Failed to emit LLVM IR");
    }
  }
};

} // namespace quantum
} // namespace catalyst

std::unique_ptr<mlir::Pass> mqssci::codegen::QuantumConversionPass() {
  return std::make_unique<QuantumConversion>();
}
