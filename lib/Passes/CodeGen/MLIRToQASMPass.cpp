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
#include "Utils/DebugUtils.h"
#include "Utils/MQTCoreUtils.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

#include <llvm/ADT/StringSet.h>

namespace mqss::mqssci::codegen {

#define GEN_PASS_DEF_MLIRTOQASMPASS
#include "Passes/CodeGen/CodeGen.h.inc"

} // namespace mqss::mqssci::codegen

using namespace mlir;
using namespace llvm;

namespace {

// The QuantumComputation builder visits quantum ops as straight-line code. Any
// control flow around quantum ops, or any call, that survives the lowering
// pipeline would therefore be flattened to a single execution and silently
// produce a wrong circuit. Reject it explicitly instead.
// Returns true (after emitting an error) if such an op is found in `kernel`.
bool hasUnloweredControlFlow(mlir::func::FuncOp kernel,
                             const QuantumKernelInfo &info) {
  static const llvm::StringSet<> controlFlowOps = {
      "scf.for",          "scf.while", "scf.if",
      "scf.index_switch", "cf.br",     "cf.cond_br",
      "cf.switch",        "cc.loop",   "cc.if"};
  static const llvm::StringSet<> callOps = {"func.call", "func.call_indirect",
                                            "cc.call_callable",
                                            "cc.call_indirect_callable"};

  auto result = kernel.walk([&](mlir::Operation *op) -> mlir::WalkResult {
    llvm::StringRef name = op->getName().getStringRef();
    if (callOps.contains(name)) {
      mlir::emitError(op->getLoc())
          << "MLIRToQASM: call '" << name
          << "' was not inlined; OpenQASM output needs straight-line code";
      return mlir::WalkResult::interrupt();
    }
    if (!controlFlowOps.contains(name))
      return mlir::WalkResult::advance();

    bool containsQuantumOp =
        op->walk([&](mlir::Operation *inner) -> mlir::WalkResult {
            return (inner != op && info.OpQViewMap.count(inner))
                       ? mlir::WalkResult::interrupt()
                       : mlir::WalkResult::advance();
          }).wasInterrupted();
    if (containsQuantumOp) {
      mlir::emitError(op->getLoc())
          << "MLIRToQASM: '" << name
          << "' around quantum operations was not unrolled/folded; "
             "OpenQASM output needs straight-line code";
      return mlir::WalkResult::interrupt();
    }
    return mlir::WalkResult::advance();
  });
  return result.wasInterrupted();
}

// MQT Core starts every dump with "// i ..." / "// o ..." layout comments.
inline std::string stripMQTLayoutComments(const std::string &qasm) {
  std::string out;
  out.reserve(qasm.size());
  size_t pos = 0;
  while (pos < qasm.size()) {
    size_t eol = qasm.find('\n', pos);
    size_t end = (eol == std::string::npos) ? qasm.size() : eol + 1;
    llvm::StringRef line(qasm.data() + pos, end - pos);
    llvm::StringRef trimmed = line.rtrim();
    bool isLayout = trimmed == "// i" || trimmed.starts_with("// i ") ||
                    trimmed == "// o" || trimmed.starts_with("// o ");
    if (!isLayout)
      out.append(line.data(), line.size());
    pos = end;
  }
  return out;
}

struct MLIRToQASM
    : public mqss::mqssci::codegen::impl::MLIRToQASMPassBase<MLIRToQASM> {

public:
  MLIRToQASM() = default;

  explicit MLIRToQASM(llvm::raw_ostream &os, bool qasm3) : os(os) {
    mode = qasm3 ? "qasm3" : "qasm2";
  }

  void runOnOperation() override {
    MQSS_DEBUG("\n[Applying Pass: MLIRToQASM]\n");
    mlir::ModuleOp module = getOperation();
    auto &selector = getAnalysis<DialectAnalysisSelector>();
    if (!selector.get()) {

      llvm::errs() << "[" << getArgument() << "]"
                   << " : Invalid MLIR Dialect\n";
      signalPassFailure();
      return;
    }
    auto &analysis = *selector.get();
    if (!analysis.isValid()) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : Invalid Module Analysis\n";
      signalPassFailure();
      return;
    }

    if ((mode != "qasm2") && (mode != "qasm3")) {
      mlir::emitError(getOperation()->getLoc(),
                      "MLIRToQASM: Invalid mode passed; should be "
                      "qasm2 or qasm3");
      signalPassFailure();
      return;
    }
    const bool emit_qasm3 = (mode == "qasm3");

    for (auto [kernel, Info] : analysis.getKernelDialectInfo()) {
      if (hasUnloweredControlFlow(kernel, Info)) {
        signalPassFailure();
        return;
      }
      auto qcOrErr = createMQTQuantumComputation(kernel->getLoc(), Info);

      if (!qcOrErr) {
        mlir::emitError(kernel->getLoc(), "MLIRToQASM: ")
            << llvm::toString(qcOrErr.takeError());
        signalPassFailure();
        return;
      }
      auto &qc = *qcOrErr;
      std::string qasm;
      try {
        std::ostringstream ss;
        qc.dumpOpenQASM(ss, emit_qasm3);
        qasm = ss.str();
      } catch (const std::exception &e) {
        mlir::emitError(kernel->getLoc(),
                        "MLIRToQASM: MQT Core failed to print OpenQASM: ")
            << e.what();
        signalPassFailure();
        return;
      }
      qasm = stripMQTLayoutComments(qasm);

      // MQT Core prints PhasedRx as "r(theta,phi)" (IQM's own name for the
      // gate) but never declares it, and qelib1.inc / stdgates.inc don't
      // either. Declare it, but only if the circuit actually uses it.
      if (qasmUsesGate(qasm, "r")) {
        qasm = insertCallerGateDefinitions(qasm, {{"r", "phased_rx"}});
      }
      // stdgates.inc defines sx but not sxdg (qelib1.inc may, so QASM3 only).
      // sxdg takes no parameters, so qasmUsesGate (which looks for "name(")
      // does not apply here.
      if (emit_qasm3 && qasm.find("sxdg ") != std::string::npos) {
        qasm = insertCallerGateDefinitions(qasm, {{"sxdg", "sxdg"}});
      }
      os << qasm;
    }

    if (failed(module.verify())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
      signalPassFailure();
      return;
    }
  }

private:
  llvm::raw_ostream &os = llvm::outs();
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::codegen::MLIRToQASMPass() {
  return std::make_unique<MLIRToQASM>();
}

std::unique_ptr<mlir::Pass>
mqss::mqssci::codegen::MLIRToQASMPass(llvm::raw_ostream &os, bool qasm3) {
  return std::make_unique<MLIRToQASM>(os, qasm3);
}
