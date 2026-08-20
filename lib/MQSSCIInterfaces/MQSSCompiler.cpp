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

#include "MQSSCIInterfaces/MQSSCompiler.h"

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"
#include "Passes/Transforms/Transforms.h"
#include "Utils/DebugUtils.h"
#include "Utils/Error.h"
#include "mlir/IR/DialectRegistry.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Parser/Parser.h>
#include <optional>
#include <sstream>
#include <string>

namespace {
// Builds the DialectRegistry MQSSCompiler needs. Kept separate from
// mqssDialectRegistry() so the latter can lazily construct it exactly once.
mlir::DialectRegistry buildDialectRegistry() {
  mlir::DialectRegistry registry;
  mqssci::opt::registerMQSSDialects(registry);
  return registry;
};

// Returns the shared dialect registry, built on first use. `static` here
// guarantees both single initialization (across all MQSSCompiler instances,
// not just one) and thread-safety, per C++11's "magic statics" guarantee.
const mlir::DialectRegistry &mqssDialectRegistry() {
  static const auto &registry = buildDialectRegistry();
  return registry;
}

} // namespace

// Parses the MLIR file at `path` into a module.
mlir::OwningOpRef<mlir::ModuleOp>
mqssci::MQSSCompiler::parseModuleFromFile(const std::filesystem::path &path,
                                          mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path.string(), &context);
}

// Parses `source` (raw MLIR text already in memory) into a module.
mlir::OwningOpRef<mlir::ModuleOp>
mqssci::MQSSCompiler::parseModuleFromSource(llvm::StringRef source,
                                            mlir::MLIRContext &context) {
  return mlir::parseSourceString<mlir::ModuleOp>(source, &context);
}

// Shared pipeline core used by both compile() and compileSource(): runs
// optimization, qubit mapping, basis conversion, and lowering on an
// already-parsed module.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqssci::MQSSCompiler::compileImpl(
    mlir::OwningOpRef<mlir::ModuleOp> module, mlir::MLIRContext &context,
    const std::string &backend_name,
    const std::vector<std::string> &native_gates,
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>
        &qubit_connectivity,
    const CompilerOptions &opts) {

  // 3. Build a pass manager and add a preset optimization pipeline
  // Default pipeline is set to -O1.
  mlir::PassManager pm(&context);
  switch (opts.optimization_level) {
  case OptLevel::O1:
    mqssci::opt::O1(pm);
    break;
  case OptLevel::O2:
    mqssci::opt::O2(pm);
    break;
  case OptLevel::O3:
    mqssci::opt::O3(pm);
    break;
  default:
    mqssci::opt::O1(pm);
    break;
  }

  if (mlir::failed(pm.run(*module))) {
    mlir::emitError(module->getLoc(), "Compiler: Optimization Pipeline failed");
    return std::nullopt;
  }

  // 4. Perform Qubit Mapping
  pm.addPass(mqssci::opt::CommonMappingPass(qubit_connectivity));
  if (mlir::failed(pm.run(*module))) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "Compiler: Qubit Mapping failed!");
    return std::nullopt;
  }

  // 5. Perform native gate decomposition
  // If a supported/known back-end is provided, perform
  // decomposition using the known native-gate set. Else,
  // use the user provided native-gate set.
  BasisConversionPassOptions conv_opts;
  conv_opts.gates = "";
  if (!backend_name.empty()) {
    if (backend_name == "iqm") {
      conv_opts.gates = "phased_rx,cz";
    } else if (backend_name == "planqc") {
      conv_opts.gates = "rx,cz,rz";
    } else if (backend_name == "wmi") {
      conv_opts.gates = "cz,x,y,rz";
    } else {
      mlir::emitError(
          module->getLoc(),
          "Unsupported backend name! Only iqm, planqc, wmi supported!");
      return std::nullopt;
    }

    pm.addPass(mqssci::codegen::createBasisConversionPass(conv_opts));

  } else if (!native_gates.empty()) {
    for (unsigned i = 0; i < native_gates.size(); i++) {
      conv_opts.gates += native_gates[i];
      if (i != native_gates.size() - 1)
        conv_opts.gates += ",";
    }
    pm.addPass(mqssci::codegen::createBasisConversionPass(conv_opts));
  } else {
    MQSS_DEBUG("No back-end name or native gate-set provided! Skipping "
               "BasisConversion!");
  }

  // 6. Lower the optimized module to OpenQASM 2 or QIR
  std::string result;
  llvm::raw_string_ostream os(result);

  switch (opts.result_format) {
  // cudaq::translateToOpenQASM walks the module's full call graph and emits
  // every non-entrypoint func::FuncOp as a "gate name(...) { ... }" block,
  // whether or not anything in the final circuit still calls it. Strip these
  // out so the returned OpenQASM2 is just the entry point's circuit body.
  case OPENQASM2:
    pm.addPass(mqssci::codegen::QuakeToQASM2Pass(os));
    if (mlir::failed(pm.run(*module))) {
      mlir::emitError(mlir::UnknownLoc::get(&context),
                      "Compiler: Conversion of Quake to QASM2 failed");
      return std::nullopt;
    }
    result = stripSpuriousGateDefs(result);
    break;
  case QIR:
  case QIRBASE:
  case QIRADAPTIVE:
  case QIRFULL: {
    auto result_type_string = resulttype_tostring(opts.result_format);
    auto convertto = tryProcessQIRLoweringOpts(result_type_string);
    if (!convertto) {
      mlir::emitError(module->getLoc(),
                      "invalid QIR lowering options: " + opts.result_format);
      return std::nullopt;
    }
    mqssci::opt::QIRConversionPipeline(pm, *convertto, os);
    if (mlir::failed(pm.run(*module))) {
      mlir::emitError(mlir::UnknownLoc::get(&context),
                      "Compiler: Conversion of Quake to " + result_type_string +
                          " failed");
      return std::nullopt;
    }
    break;
  }
  default:
    mlir::emitError(
        module->getLoc(),
        "Unsupported exchange format! Only OpenQASM2 and QIR supported!");
    return std::nullopt;
  }

  return result;
}

// Parses the input circuit from a file on disk, then runs it through the
// shared compileImpl() pipeline.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqssci::MQSSCompiler::compile(
    const std::filesystem::path &input_circuit_path,
    const std::string &backend_name,
    const std::vector<std::string> &native_gates,
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>
        &qubit_connectivity,
    const CompilerOptions &opts) {

  // 1. Get the shared dialect registry and use it to create a fresh
  // MLIRContext for this compilation. The context is deliberately not
  // cached/reused across calls, so each compilation is isolated and one
  // call's state can't leak into or bloat another's.
  auto &registry = mqssDialectRegistry();
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  // 2. Parse the MLIR file into a module.
  auto module = parseModuleFromFile(input_circuit_path, context);
  if (!module) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "failed to parse MLIR file");
    return std::nullopt;
  }

  return compileImpl(std::move(module), context, backend_name, native_gates,
                     qubit_connectivity, opts);
}

// Parses the input circuit from raw MLIR source text already in memory,
// then runs it through the shared compileImpl() pipeline.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqssci::MQSSCompiler::compileSource(
    const std::string &input_circuit, const std::string &backend_name,
    const std::vector<std::string> &native_gates,
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>
        &qubit_connectivity,
    const CompilerOptions &opts) {

  // 1. Get the shared dialect registry and use it to create a fresh
  // MLIRContext for this compilation. The context is deliberately not
  // cached/reused across calls, so each compilation is isolated and one
  // call's state can't leak into or bloat another's.
  auto &registry = mqssDialectRegistry();
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  // 2. Parse the MLIR source text into a module.
  auto module = parseModuleFromSource(input_circuit, context);
  if (!module) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "failed to parse MLIR source");
    return std::nullopt;
  }

  return compileImpl(std::move(module), context, backend_name, native_gates,
                     qubit_connectivity, opts);
}

// Removes any "gate ... { ... }" block from OpenQASM2 text emitted by
// cudaq::translateToOpenQASM (see TranslateToOpenQASM.cpp) — see the comment
// at the call site in compileImpl() for why these blocks can be present.
std::string
mqssci::MQSSCompiler::stripSpuriousGateDefs(const std::string &qasm) {
  std::istringstream stream(qasm);
  std::ostringstream result;
  std::string line;
  bool inGateBlock = false;
  while (std::getline(stream, line)) {
    if (line.find("gate ") != std::string::npos &&
        line.find("{") != std::string::npos) {
      inGateBlock = true;
      continue;
    }
    if (inGateBlock) {
      if (line.find("}") != std::string::npos) {
        inGateBlock = false;
      }
      continue;
    }
    result << line << "\n";
  }
  return result.str();
}
