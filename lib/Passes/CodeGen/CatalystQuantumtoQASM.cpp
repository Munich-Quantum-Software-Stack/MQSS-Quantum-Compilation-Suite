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

namespace mqss::mqssci::codegen {

#define GEN_PASS_DEF_CATALYSTTOQASMPASS
#include "Passes/CodeGen/CodeGen.h.inc"

} // namespace mqss::mqssci::codegen

using namespace mlir;
using namespace llvm;

namespace {

struct CatalystToQASM
    : public mqss::mqssci::codegen::impl::CatalystToQASMPassBase<
          CatalystToQASM> {

public:
  CatalystToQASM() = default;

  explicit CatalystToQASM(llvm::raw_ostream &os, bool qasm3) : os(os) {
    mode = qasm3 ? "qasm3" : "qasm2";
  }

  void runOnOperation() override {
    MQSS_DEBUG("\n[Applying Pass: CatalystToQASM]\n");
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
                      "CatalystQuantumToQASM: Invalid mode passed; should be "
                      "qasm2 or qasm3");
      signalPassFailure();
      return;
    }
    const bool emit_qasm3 = (mode == "qasm3");

    for (auto [kernel, Info] : analysis.getKernelDialectInfo()) {
      auto qcOrErr = createMQTQuantumComputation(kernel->getLoc(), Info);

      if (!qcOrErr) {
        mlir::emitError(kernel->getLoc(),
                        "CatalystQuantumToQASM: Could not create MQT Core's "
                        "qc::QuantumComputation Obj");
        signalPassFailure();
        return;
      }
      auto qc = *qcOrErr;
      std::ostringstream ss;
      qc.dumpOpenQASM(ss, emit_qasm3);
      std::string qasm = ss.str();

      // MQT Core prints PhasedRx as "r(theta,phi)" (IQM's own name for the
      // gate) but never declares it, and qelib1.inc / stdgates.inc don't
      // either. Declare it, but only if the circuit actually uses it.
      if (qasmUsesGate(qasm, "r")) {
        const std::vector<std::pair<std::string, std::string>> aliases = {
            {"r", "phased_rx"}};
        qasm = insertCallerGateDefinitions(qasm, aliases);
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

std::unique_ptr<mlir::Pass> mqss::mqssci::codegen::CatalystToQASMPass() {
  return std::make_unique<CatalystToQASM>();
}

std::unique_ptr<mlir::Pass>
mqss::mqssci::codegen::CatalystToQASMPass(llvm::raw_ostream &os, bool qasm3) {
  return std::make_unique<CatalystToQASM>(os, qasm3);
}
