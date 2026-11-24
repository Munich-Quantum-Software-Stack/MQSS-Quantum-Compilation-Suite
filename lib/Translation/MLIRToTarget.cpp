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

#include "Translation/MLIRToTarget.h"

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Pipelines/Pipelines.h"
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/TransformPasses.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Tools/mlir-translate/Translation.h"

#include <functional>
#include <string>
#include <utility>

using namespace mlir;

namespace {

using PipelineBuilder =
    std::function<void(OpPassManager &, llvm::raw_ostream &)>;

// Builds a pipeline with `build` and runs it on `op`. The last pass of every
// pipeline writes its text output to `os`, which is mlir-translate's output.
LogicalResult runPipeline(Operation *op, llvm::raw_ostream &os,
                          const PipelineBuilder &build) {
  PassManager pm(op->getContext());
  build(pm, os);
  return pm.run(op);
}

TranslateFromMLIRFunction makeTranslation(PipelineBuilder build) {
  return [build = std::move(build)](Operation *op,
                                    llvm::raw_ostream &os) -> LogicalResult {
    return runPipeline(op, os, build);
  };
}

// `profile` is "<name>:<version>", e.g. "qir-base:2.0". It is validated when
// the translation runs so a bad value is reported as a diagnostic.
TranslateFromMLIRFunction makeQIRTranslation(std::string profile) {
  return [profile = std::move(profile)](
             Operation *op, llvm::raw_ostream &os) -> LogicalResult {
    auto converted = tryProcessQIRLoweringOpts(profile);
    if (!converted)
      return op->emitError() << "invalid QIR lowering options: " << profile;
    return runPipeline(op, os, [&](OpPassManager &pm, llvm::raw_ostream &out) {
      mqss::mqssci::opt::QIRConversionPipeline(pm, *converted, out);
    });
  };
}

// For pipelines whose result is an MLIR module rather than text.
TranslateFromMLIRFunction makeModuleTranslation(PipelineBuilder build) {
  return [build = std::move(build)](Operation *op,
                                    llvm::raw_ostream &os) -> LogicalResult {
    if (failed(runPipeline(op, os, build)))
      return failure();
    op->print(os);
    os << "\n";
    return success();
  };
}

// The input module is parsed by mlir-translate, so every dialect the MQSS
// pipelines can consume must be registered.
void registerDialects(DialectRegistry &registry) {
  mqss::mqssci::opt::registerMQSSDialects(registry);
}

} // namespace

// Note: the name and description passed to TranslateFromMLIRRegistration must
// outlive the registration, so they are string literals.
void mqss::mqssci::translation::registerMLIRToTargetTranslations() {

  static TranslateFromMLIRRegistration llvmIR(
      "llvm-dialect-to-llvm-ir", "Translate an LLVM-dialect module to LLVM IR",
      makeTranslation([](OpPassManager &pm, llvm::raw_ostream &os) {
        pm.addPass(mqss::mqssci::codegen::LLVMDialectToLLVMIRPass(os));
      }),
      registerDialects);

  static TranslateFromMLIRRegistration quantum(
      "convert-quantum-to-llvm",
      "Translate Catalyst-quantum dialect to LLVM dialect",
      makeModuleTranslation([](OpPassManager &pm, llvm::raw_ostream &os) {
        pm.addPass(mqss::mqssci::codegen::QuantumConversionPass());
      }),
      registerDialects);

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss::mqssci::codegen::QuantumConversionPass();
  });

  static TranslateFromMLIRRegistration qasm2(
      "mlir-to-qasm2",
      "Translate a Quake or Catalyst-quantum module to OpenQASM 2",
      makeTranslation([](OpPassManager &pm, llvm::raw_ostream &os) {
        mqss::mqssci::opt::QASMLoweringPipeline(pm, /*qasm3=*/false, os);
      }),
      registerDialects);

  static TranslateFromMLIRRegistration qasm3(
      "mlir-to-qasm3",
      "Translate a Quake or Catalyst-quantum module to OpenQASM 3",
      makeTranslation([](OpPassManager &pm, llvm::raw_ostream &os) {
        mqss::mqssci::opt::QASMLoweringPipeline(pm, /*qasm3=*/true, os);
      }),
      registerDialects);

  static TranslateFromMLIRRegistration iqmJson(
      "mlir-to-iqm-json", "Translate a Quake module to IQM JSON",
      makeTranslation([](OpPassManager &pm, llvm::raw_ostream &os) {
        pm.addPass(mqss::mqssci::opt::createExpandMeasurementsPass());
        pm.addPass(mqss::mqssci::codegen::QuakeDialectToIQMJSONPass(os));
      }),
      registerDialects);

  static TranslateFromMLIRRegistration qir(
      "mlir-to-qir", "Translate a Quake module to QIR (full profile, 2.0)",
      makeQIRTranslation("qir:2.0"), registerDialects);

  static TranslateFromMLIRRegistration qirBase(
      "mlir-to-qir-base", "Translate a Quake module to QIR (base profile, 2.0)",
      makeQIRTranslation("qir-base:2.0"), registerDialects);

  static TranslateFromMLIRRegistration qirAdaptive(
      "mlir-to-qir-adaptive",
      "Translate a Quake module to QIR (adaptive profile, 2.0)",
      makeQIRTranslation("qir-adaptive:2.0"), registerDialects);

  static TranslateFromMLIRRegistration qirFull(
      "mlir-to-qir-full", "Translate a Quake module to QIR (full profile, 2.0)",
      makeQIRTranslation("qir-full:2.0"), registerDialects);
}
