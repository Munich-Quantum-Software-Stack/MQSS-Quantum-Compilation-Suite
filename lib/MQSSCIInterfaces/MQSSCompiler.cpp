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
#include "Passes/Transforms/TransformPasses.h"
#include "Passes/Verification/Instrumentation.h"
#include "Translation/QASM2ToQuake.h"
#include "Utils/DebugUtils.h"
#include "Utils/Error.h"
#include "mlir/IR/DialectRegistry.h"

#include <cctype>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Parser/Parser.h>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {
// Builds the DialectRegistry MQSSCompiler needs. Kept separate from
// mqssDialectRegistry() so the latter can lazily construct it exactly once.
mlir::DialectRegistry buildDialectRegistry() {
  mlir::DialectRegistry registry;
  mqss::mqssci::opt::registerMQSSDialects(registry);
  return registry;
};

// Returns the shared dialect registry, built on first use. `static` here
// guarantees both single initialization (across all MQSSCompiler instances,
// not just one) and thread-safety, per C++11's "magic statics" guarantee.
const mlir::DialectRegistry &mqssDialectRegistry() {
  static const auto &registry = buildDialectRegistry();
  return registry;
}

// True if `text` is an OpenQASM program, i.e. its first token (after
// whitespace and `//` comments) is the `OPENQASM` version header. MLIR text
// can never start that way, so this doesn't misfire on Quake/Catalyst input.
bool isQASM2(llvm::StringRef text) {
  for (text = text.ltrim(); text.starts_with("//"); text = text.ltrim())
    text = text.drop_until([](char c) { return c == '\n'; });
  return text.starts_with("OPENQASM");
}

} // namespace

// Parses the circuit file at `path` into a module: OpenQASM 2 is translated
// to Quake, anything else is parsed as MLIR.
mlir::OwningOpRef<mlir::ModuleOp>
mqss::mqssci::MQSSCompiler::parseModuleFromFile(
    const std::filesystem::path &path, mlir::MLIRContext &context) {
  auto buffer = llvm::MemoryBuffer::getFile(path.string());
  if (!buffer) {
    mlir::emitError(mlir::UnknownLoc::get(&context))
        << "cannot read '" << path.string()
        << "': " << buffer.getError().message();
    return nullptr;
  }
  if (isQASM2((*buffer)->getBuffer()))
    return translation::translateQASM2ToQuake((*buffer)->getBuffer(), context);
  // Parsed from the path (not the buffer above) so MLIR diagnostics keep the
  // file name.
  return mlir::parseSourceFile<mlir::ModuleOp>(path.string(), &context);
}

// Parses `source` (raw MLIR text already in memory) into a module.
mlir::OwningOpRef<mlir::ModuleOp>
mqss::mqssci::MQSSCompiler::parseMLIRSource(llvm::StringRef source,
                                            mlir::MLIRContext &context) {
  return mlir::parseSourceString<mlir::ModuleOp>(source, &context);
}

// Parses `source` (circuit text already in memory) into a module: OpenQASM 2
// is translated to Quake, anything else is parsed as MLIR.
mlir::OwningOpRef<mlir::ModuleOp>
mqss::mqssci::MQSSCompiler::parseModuleString(llvm::StringRef source,
                                              mlir::MLIRContext &context) {
  if (isQASM2(source))
    return translation::translateQASM2ToQuake(source, context);
  return parseMLIRSource(source, context);
}

// Shared pipeline core used by both compile() and compileSource(): runs
// optimization, qubit mapping, basis conversion, and lowering on an
// already-parsed module.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqss::mqssci::MQSSCompiler::compileImpl(
    mlir::OwningOpRef<mlir::ModuleOp> module, mlir::MLIRContext &context,
    const std::string &backend_name,
    const std::vector<std::string> &native_gates,
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>
        &qubit_connectivity,
    const CompilerOptions &opts) {

  // 3. Build a pass manager and add a preset optimization pipeline
  // Default pipeline is set to -O1.
  mlir::PassManager pmOptimize(&context);
  switch (opts.optimization_level) {
  case OptLevel::O1:
    mqss::mqssci::opt::O1(pmOptimize);
    break;
  case OptLevel::O2:
    mqss::mqssci::opt::O2(pmOptimize);
    break;
  case OptLevel::O3:
    mqss::mqssci::opt::O3(pmOptimize);
    break;
  default:
    mqss::mqssci::opt::O1(pmOptimize);
    break;
  }

  if (mlir::failed(pmOptimize.run(*module))) {
    mlir::emitError(module->getLoc(), "Compiler: Optimization Pipeline failed");
    return std::nullopt;
  }

  mlir::PassManager pmMap(&context);
  // 4. Perform Qubit Mapping
  pmMap.addPass(mqss::mqssci::opt::CommonMappingPass(qubit_connectivity));
  if (mlir::failed(pmMap.run(*module))) {
    mlir::emitError(module->getLoc(), "Compiler: Qubit Mapping failed!");
    return std::nullopt;
  }

  // 5. Perform native gate decomposition using the caller's native-gate
  // set. For the LRZ backend, also record which of the caller's gate names
  // BasisConversionPass will resolve to its own mnemonic (e.g. "r" ->
  // "prx"), so the QASM2 output can be renamed back to the caller's
  // spelling afterwards (see restoreCallerGateNames).
  mlir::PassManager pmConvert(&context);
  BasisConversionPassOptions conv_opts;
  conv_opts.gates = "";
  std::vector<std::pair<std::string, std::string>>
      appliedAliases; // (caller spelling, QASM2 gate name)
  if (backend_name == "LRZ") {
    for (const auto &gate : native_gates) {
      llvm::StringRef resolvedMnemonic = resolveMnemonicAlias(gate);
      if (resolvedMnemonic != gate)
        appliedAliases.emplace_back(
            gate, mnemonicToQasmGateName(resolvedMnemonic).str());
    }
  }
  if (!native_gates.empty()) {
    for (unsigned i = 0; i < native_gates.size(); i++) {
      conv_opts.gates += native_gates[i];
      if (i != native_gates.size() - 1)
        conv_opts.gates += ",";
    }
    pmConvert.addPass(
        mqss::mqssci::codegen::createBasisConversionPass(conv_opts));
  } else {
    mlir::emitError(module->getLoc(),
                    "Compiler: No native gate-set provided! BasisConversion "
                    "and or Verification failed!");
    return nullopt;
  }

  // Some BasisConversionPass rewrite rules build a gate's angle out of an
  // arithmetic expression (e.g. arith.negf/arith.divf on a prior operand)
  // rather than a literal constant. Downstream lowering (e.g.
  // QuakeDialectToIQMJSONPass, via cudaq::translateToIQMJson) expects every
  // rotation parameter to already be a literal constant it can read as a
  // double, and asserts rather than failing gracefully if it isn't. Folding
  // here, right after BasisConversion and before any lowering pass runs,
  // collapses those expressions back down to literal constants regardless of
  // which result_format is requested below.
  pmConvert.addPass(mlir::createCSEPass());
  pmConvert.addPass(mlir::createCanonicalizerPass());

  // 6. Perform verification via Circuit Equivalence Check after each pass
  // in the pass pipeline.
  if (opts.verify) {
    llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy> snapshot;
    pmConvert.addInstrumentation(
        std::make_unique<mqss::mqssci::verify::VerifyPassInstrumentation>(
            std::move(snapshot)));
  }

  if (mlir::failed(pmConvert.run(*module))) {
    mlir::emitError(module->getLoc(),
                    "Compiler: BasisConversion and or Verification failed!");
    return std::nullopt;
  }

  // 7. Lower the optimized module to OpenQASM 2 or QIR
  std::string result;
  llvm::raw_string_ostream os(result);
  mlir::PassManager pmLower(&context);
  switch (opts.result_format) {
  // cudaq::translateToOpenQASM walks the module's full call graph and emits
  // every non-entrypoint func::FuncOp as a "gate name(...) { ... }" block,
  // whether or not anything in the final circuit still calls it. Strip these
  // out so the returned OpenQASM2 is just the entry point's circuit body.
  case OPENQASM2:
    pmLower.addPass(mqss::mqssci::codegen::QuakeToQASM2Pass(os));
    if (mlir::failed(pmLower.run(*module))) {
      mlir::emitError(module->getLoc(),
                      "Compiler: Conversion of Quake to QASM2 failed");
      return std::nullopt;
    }
    result =
        restoreCallerGateNames(stripSpuriousGateDefs(result), appliedAliases);
    if (backend_name == "LRZ") {
      result = insertCallerGateDefinitions(result, appliedAliases);
    }
    break;
  case IQMJSON:
    pmLower.addPass(mqss::mqssci::opt::createExpandMeasurementsPass());
    pmLower.addPass(mqss::mqssci::codegen::QuakeDialectToIQMJSONPass(os));
    if (mlir::failed(pmLower.run(*module))) {
      mlir::emitError(module->getLoc(),
                      "Compiler: Conversion of Quake to IQMJSON failed");
      return std::nullopt;
    }
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
    mqss::mqssci::opt::QIRConversionPipeline(pmLower, *convertto, os);
    if (mlir::failed(pmLower.run(*module))) {
      mlir::emitError(module->getLoc(), "Compiler: Conversion of Quake to " +
                                            result_type_string + " failed");
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
std::optional<std::string> mqss::mqssci::MQSSCompiler::compile(
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

  // 2. Parse the circuit file (MLIR or OpenQASM 2) into a module.
  auto module = parseModuleFromFile(input_circuit_path, context);
  if (!module) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "failed to parse input circuit file");
    return std::nullopt;
  }

  return compileImpl(std::move(module), context, backend_name, native_gates,
                     qubit_connectivity, opts);
}

// Parses the input circuit from raw MLIR source text already in memory,
// then runs it through the shared compileImpl() pipeline.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqss::mqssci::MQSSCompiler::compileSource(
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

  // 2. Parse the source text (MLIR or OpenQASM 2) into a module.
  auto module = parseModuleString(input_circuit, context);
  if (!module) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "failed to parse input circuit source");
    return std::nullopt;
  }

  return compileImpl(std::move(module), context, backend_name, native_gates,
                     qubit_connectivity, opts);
}

// Translates OpenQASM 2 source text into Quake and returns the module's
// textual MLIR form.
std::optional<std::string>
mqss::mqssci::MQSSCompiler::translateQASM2ToQuake(const std::string &qasm2) {
  auto &registry = mqssDialectRegistry();
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  auto module =
      mqss::mqssci::translation::translateQASM2ToQuake(qasm2, context);
  if (!module || mlir::failed(module->verify()))
    return std::nullopt;

  std::string result;
  llvm::raw_string_ostream os(result);
  module->print(os);
  return result;
}

// Removes any "gate ... { ... }" block from OpenQASM2 text emitted by
// cudaq::translateToOpenQASM (see TranslateToOpenQASM.cpp) — see the comment
// at the call site in compileImpl() for why these blocks can be present.
std::string
mqss::mqssci::MQSSCompiler::stripSpuriousGateDefs(const std::string &qasm) {
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

// Renames each gate call in OpenQASM2 text from its QASM2 gate name back to
// the caller's own spelling, e.g. "phased_rx(" -> "r(". Only aliases this
// specific compile actually used are applied, so a caller that genuinely
// calls the gate "prx" is never touched. A match is only replaced when it
// starts a gate name, so e.g. a hypothetical "cphased_rx(" is left alone.
std::string mqss::mqssci::MQSSCompiler::restoreCallerGateNames(
    std::string qasm,
    const std::vector<std::pair<std::string, std::string>> &appliedAliases) {
  auto isIdentChar = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  };
  for (const auto &[callerSpelling, qasmName] : appliedAliases) {
    const std::string needle = qasmName + "(";
    const std::string replacement = callerSpelling + "(";
    for (size_t pos = qasm.find(needle); pos != std::string::npos;
         pos = qasm.find(needle, pos)) {
      if (pos > 0 && isIdentChar(qasm[pos - 1])) {
        pos += needle.size();
        continue;
      }
      qasm.replace(pos, needle.size(), replacement);
      pos += replacement.size();
    }
  }
  return qasm;
}

// Declares each caller-spelled gate that restoreCallerGateNames introduced
// (e.g. "r") right after the qelib1.inc include, since those names aren't
// defined there. Gates qelib1.inc already defines (e.g. "cz") have no entry
// and are left alone; each gate is declared at most once.
std::string mqss::mqssci::MQSSCompiler::insertCallerGateDefinitions(
    std::string qasm,
    const std::vector<std::pair<std::string, std::string>> &appliedAliases) {
  std::string defs;
  std::unordered_set<std::string> alreadyDefined;
  for (const auto &[callerSpelling, qasmName] : appliedAliases) {
    auto body = qasmGateDefinitionBody(qasmName);
    if (body.empty() || !alreadyDefined.insert(callerSpelling).second)
      continue;
    defs += "gate " + callerSpelling + body.str() + "\n";
  }
  if (defs.empty())
    return qasm;

  // Insert after the include line if present, else after the version line.
  size_t pos = qasm.find("include \"qelib1.inc\";");
  if (pos == std::string::npos)
    pos = qasm.find("OPENQASM 2.0;");
  if (pos == std::string::npos)
    return defs + qasm;
  size_t eol = qasm.find('\n', pos);
  qasm.insert(eol == std::string::npos ? qasm.size() : eol + 1, defs);
  return qasm;
}
