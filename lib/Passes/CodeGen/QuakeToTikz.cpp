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
  date   January 2025
  version 1.0
  brief
    MLIR/Quake pass that dump a given Quake circuit into Tikz for visualization
    purposes.

*******************************************************************************
* This source code and the accompanying materials are made available under    *
* the terms of the Apache License 2.0 which accompanies this distribution.    *
******************************************************************************/

#include "Passes/CodeGen.hpp"
#include "Support/CodeGen/Quake.hpp"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Support/Plugin.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include <iomanip>
#include <regex>

using namespace mlir;
using namespace mqss::support::quakeDialect;

void dumpQuakeOperationToTikz(
    mlir::Operation *op, std::vector<std::vector<std::string>> &qubitLines) {
  if (op->getDialect()->getNamespace() != "quake")
    return; // do nothing if it is not a quake operation
  if (isa<quake::AllocaOp>(op) || isa<quake::ExtractRefOp>(op))
    return; // do nothing if the next operation is a qubit allocation
  std::string gateName = std::string(op->getName().getStringRef());
  size_t pos = gateName.find("quake.");
  if (pos != std::string::npos) {
    gateName.erase(pos, 6); // 6 is the length of "quake."
  }
#ifdef DEBUG
  llvm::outs() << "Operation: " << gateName << "\n";
#endif
  std::vector<double> parameters;
  std::vector<int> targets;
  std::vector<int> controls;
  std::vector<int> measurements;
  bool isAdj = false;
  if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op)) {
    for (auto operand : op->getOperands()) {
      if (operand.getType().isa<quake::RefType>()) {
        int qubitIndex =
            extractIndexFromQuakeExtractRefOp(operand.getDefiningOp());
        assert(qubitIndex != -1 && "Non valid qubit index for measurement!");
        measurements.push_back(qubitIndex);
        qubitLines[qubitIndex].push_back("\\meter{}");
      } else if (operand.getType().isa<quake::VeqType>()) {
        auto qvecType = operand.getType().dyn_cast<quake::VeqType>();
        for (int i = 0; i < qubitLines.size(); i++) {
          measurements.push_back(i);
          qubitLines[i].push_back("\\meter{}");
        }
      }
    }
  } else {
    auto gate = dyn_cast<quake::OperatorInterface>(op);
    parameters = getParametersValues(gate.getParameters());
    targets = getIndicesOfValueRange(gate.getTargets());
    controls = getIndicesOfValueRange(gate.getControls());
    isAdj = gate.isAdj();
#ifdef DEBUG
    llvm::outs() << "\tParameters: " << parameters.size()
                 << " Targets: " << targets.size()
                 << " Controls :" << controls.size() << "\n";
#endif
  }
  // empty fill those qubits that are not used by the current targets, controls
  // and measurements
  for (int i = 0; i < qubitLines.size(); i++) {
    if (!(std::find(targets.begin(), targets.end(), i) != targets.end()) &&
        !(std::find(controls.begin(), controls.end(), i) != controls.end()) &&
        !(std::find(measurements.begin(), measurements.end(), i) !=
          measurements.end())) {
      qubitLines[i].push_back("\\qw");
    }
  }
  // exit after insert measurements
  if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op))
    return;

  if (isa<quake::SwapOp>(op)) {
    assert(targets.size() == 2 &&
           "At the moment the SWAP only works on two targets...");
    qubitLines[targets[0]].push_back(
        "\\swap{" + std::to_string(targets[1] - targets[0]) + "}");
    qubitLines[targets[1]].push_back("\\swap{}");
    return; // special handle for swap because it has multiple targets
  }
  // iterate over all the targets and insert it
  for (int target_qubit : targets) {
    std::string labelGate = gateName;
    // rename in case of rotations for sake of better readability
    // phased_rx -> prx
    labelGate = std::regex_replace(labelGate, std::regex("phased_"), "p");
    // append dagger in case of adj
    if (isAdj)
      labelGate = labelGate + "\\textsuperscript{\\textdagger}";
    // if the gate has parameters, annotate into the label
    if (parameters.size() > 0)
      labelGate += "(";
    int countParam = 0;
    for (double parameter : parameters) {
      std::ostringstream tmpOss;
      tmpOss << std::fixed << std::setprecision(2) << parameter;
      labelGate += std::string(tmpOss.str());
      if (countParam != parameters.size() - 1)
        labelGate += " , ";
      countParam++;
    }
    if (parameters.size() > 0)
      labelGate += ")";
    // annotating the target qubit
    qubitLines[target_qubit].push_back("\\gate{" + labelGate + "}");
  }
  // CHECK: I am assuming at the moment multiple controls and single targets
  // the opposite case, single control and multiple target should not be a valid
  // gate
  int targetGate = targets[0];
  for (int control_qubit : controls)
    qubitLines[control_qubit].push_back(
        "\\ctrl{" + std::to_string(targetGate - control_qubit) + "}");
}

namespace {

class QuakeToTikzPass
    : public PassWrapper<QuakeToTikzPass, OperationPass<func::FuncOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(QuakeToTikzPass)

  QuakeToTikzPass(llvm::raw_ostream &ostream) : outputStream(ostream) {}

  llvm::StringRef getArgument() const override {
    return "convert-quake-to-tikz";
  }
  llvm::StringRef getDescription() const override {
    return "Lower Quake Operations to LaTeX TiKz";
  };

  void runOnOperation() override {
    auto circuit = getOperation();
    // Get the function name
    StringRef funcName = circuit.getName();
    if (!(funcName.find(std::string(CUDAQ_PREFIX_FUNCTION)) !=
          std::string::npos))
      return; // do nothing if the function is not cudaq kernel

    std::map<int, int> measurements; // key: qubit, value register index
    int numQubits = getNumberOfQubits(circuit);
    int numBits = getNumberOfClassicalBits(circuit, measurements);
    std::vector<std::vector<std::string>> qubitTikz(numQubits);
    for (int i = 0; i < numQubits; i++)
      qubitTikz[i].push_back({"\\lstick{\\ket{0}}"});
    circuit.walk(
        [&](Operation *op) { dumpQuakeOperationToTikz(op, qubitTikz); });
#ifdef DEBUG
    llvm::outs() << "Num qubits " << numQubits << "\n";
#endif
    // dumping the lines into the ostream
    outputStream << "\\begin{quantikz}\n";
    int countQubit = 0;
    for (auto qubit : qubitTikz) {
      int size = qubit.size();
#ifdef DEBUG
      llvm::outs() << "Ops size " << size << "\n";
#endif
      int countOps = 0;
      outputStream << "\t";
      for (auto op : qubit) {
        outputStream << op;
        if (countOps != size - 1)
          outputStream << "\t&\t";
        countOps++;
      }
      if (countQubit != numQubits - 1)
        outputStream << "\\\\\n";
      else
        outputStream << "\n";
      countQubit++;
    }
    outputStream << "\\end{quantikz}";
  }

private:
  llvm::raw_ostream &outputStream; // Store the tikz circuit
};

} // namespace

std::unique_ptr<mlir::Pass>
mqss::opt::createQuakeToTikzPass(llvm::raw_ostream &ostream) {
  return std::make_unique<QuakeToTikzPass>(ostream);
}
