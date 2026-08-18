
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
#include <mlir/IR/BuiltinOps.h>
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
  mqss::opt::registerMQSSDialects(registry);
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

// Implementation of the main "compile" interface.
// Note: Currently only cudaq-quake dialect is supported
std::optional<std::string> mqss::MQSSCompiler::compile(
    const std::string &input_circuit_path, const std::string &backend_name,
    const std::vector<std::string> &native_gates,
    const std::unordered_map<int, int> &qubit_connectivity,
    const CompilerOptions &opts) {

  // 1. Get the shared dialect registry and use it to create a fresh
  // MLIRContext for this compilation. The context is deliberately not
  // cached/reused across calls, so each compilation is isolated and one
  // call's state can't leak into or bloat another's.
  auto &registry = mqssDialectRegistry();
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  // 2. Parse the MLIR file into a module
  auto module =
      mlir::parseSourceFile<mlir::ModuleOp>(input_circuit_path, &context);
  if (!module) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "failed to parse MLIR file");
    return std::nullopt;
  }

  // 3. Build a pass manager and add a preset optimization pipeline
  // Default pipeline is set to -O1.
  mlir::PassManager pm(&context);
  switch (opts.optimization_level) {
  case 1:
    mqss::opt::O1(pm);
    break;
  case 2:
    mqss::opt::O2(pm);
    break;
  case 3:
    mqss::opt::O3(pm);
    break;
  default:
    mqss::opt::O1(pm);
    break;
  }

  if (mlir::failed(pm.run(*module))) {
    mlir::emitError(module->getLoc(), "Compiler: Optimization Pipeline failed");
    return std::nullopt;
  }

  // 4. Perform Qubit Mapping
  pm.addPass(mqss::opt::CommonMappingPass(qubit_connectivity));
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

    pm.addPass(mqss::codegen::createBasisConversionPass(conv_opts));

  } else if (!native_gates.empty()) {
    for (unsigned i = 0; i < native_gates.size(); i++) {
      conv_opts.gates += native_gates[i];
      if (i != native_gates.size() - 1)
        conv_opts.gates += ",";
    }
    pm.addPass(mqss::codegen::createBasisConversionPass(conv_opts));
  } else {
    MQSS_DEBUG("No back-end name or native gate-set provided! Skipping "
               "BasisConversion!");
  }

  // 6. Lower the optimized module to OpenQASM 2 or QIR
  std::string result;
  llvm::raw_string_ostream os(result);
  if (opts.result_type == "OpenQASM2")
    pm.addPass(mqss::codegen::QuakeToQASM2Pass(os));
  else if (opts.result_type.find("qir") != std::string::npos) {
    auto convertto = tryProcessQIRLoweringOpts(opts.result_type);
    if (!convertto) {
      mlir::emitError(module->getLoc(),
                      "invalid QIR lowering options: " + opts.result_type);
      return std::nullopt;
    }
    mqss::opt::QIRConversionPipeline(pm, *convertto, os);
  } else {
    mlir::emitError(
        module->getLoc(),
        "Unsupported exchange format! Only OpenQASM2 and QIR supported!");
    return std::nullopt;
  }

  if (mlir::failed(pm.run(*module))) {
    mlir::emitError(mlir::UnknownLoc::get(&context),
                    "Compiler: Conversion of Quake to " + opts.result_type +
                        " failed");
    return std::nullopt;
  }

  // cudaq::translateToOpenQASM walks the module's full call graph and emits
  // every non-entrypoint func::FuncOp as a "gate name(...) { ... }" block,
  // whether or not anything in the final circuit still calls it. Strip these
  // out so the returned OpenQASM2 is just the entry point's circuit body.
  if (opts.result_type == "OpenQASM2")
    result = stripSpuriousGateDefs(result);

  return result;
}

// Removes any "gate ... { ... }" block from OpenQASM2 text emitted by
// cudaq::translateToOpenQASM (see TranslateToOpenQASM.cpp) — see the comment
// at the call site in compile() for why these blocks can be present.
std::string mqss::MQSSCompiler::stripSpuriousGateDefs(const std::string &qasm) {
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
