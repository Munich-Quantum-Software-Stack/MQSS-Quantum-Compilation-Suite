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

#include "Utils/TransformUtils.h"
#include "ir/QuantumComputation.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "sc/exact/ExactMapper.hpp"
#include "sc/heuristic/HeuristicMapper.hpp"

#include <mlir/IR/Value.h>

using namespace mlir;

static std::optional<double> getConstantDouble(mlir::Value v) {
  auto defOp = v.getDefiningOp<mlir::arith::ConstantOp>();
  if (!defOp)
    return std::nullopt;

  auto attr = llvm::dyn_cast<mlir::FloatAttr>(defOp.getValue());
  if (!attr)
    return std::nullopt;

  return attr.getValueAsDouble();
}

static void
resolveSSAformForMeasureOps(mlir::Operation *OldMeasOp,
                            SmallVector<mlir::Value, 2> newResults) {
  // llvm::outs() << "Old meas op: " << *OldMeasOp
  //              << " results: " << OldMeasOp->getResults().size() << ","
  //              << newResults.size() << "\n";
  auto OldResults = OldMeasOp->getResults();
  for (int j = 0; j < OldResults.size(); ++j) {
    auto oldRes = OldResults[j];
    auto newRes = newResults[j];
    oldRes.replaceAllUsesWith(newRes);
  }
  assert(OldMeasOp->use_empty() &&
         "Old measurement Op should not have any users before erasing!");
  OldMeasOp->erase();
}

// Load the measurement operation within QuantumComputation
static void loadMeasureOpIntoQC(QuantumOpView qview,
                                qc::QuantumComputation &qc) {

  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((targetQubitVector.size() == 1) &&
         "Only Single Qubit Measurement Ops supported");

  auto targetQubit = targetQubitVector[0].index;
  for (auto entry : qview.measurements) {
    qc.measure(entry.QubitIndex, entry.ClassicalBitIndex);
  }
}

static void loadGates(mlir::Operation *gateOp, qc::QuantumComputation &qc,
                      Gate GateTy, SmallVector<mlir::Value, 2> params,
                      int64_t controlQubit, int64_t targetQubit1,
                      int64_t targetQubit2 = -2) {

  // llvm::outs() << "Gate Op: " << *gateOp << " , Ty: " << GateTy << "\n";

  std::optional<double> angle;
  // TODO: Load more gates into qc. Add to the following list.
  switch (GateTy) {
  case Gate::H:
  case Gate::Hadamard:
    qc.h(targetQubit1);
    break;
  case Gate::CNOT:
    qc.cx(controlQubit, targetQubit1);
    break;
  case Gate::CY:
    qc.cy(controlQubit, targetQubit1);
    break;
  case Gate::CZ:
    qc.cz(controlQubit, targetQubit1);
    break;
  case Gate::PauliX:
    qc.x(targetQubit1);
    break;
  case Gate::PauliY:
    qc.y(targetQubit1);
    break;
  case Gate::PauliZ:
    qc.z(targetQubit1);
    break;
  case Gate::S:
    qc.s(targetQubit1);
    break;
  case Gate::T:
    qc.t(targetQubit1);
    break;
  case Gate::RX:
    assert(params.size() == 1 && "RX gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rx(angle.value(), targetQubit1);
    break;
  case Gate::RY:
    assert(params.size() == 1 && "RY gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.ry(angle.value(), targetQubit1);
    break;
  case Gate::RZ:
    assert(params.size() == 1 && "RZ gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rz(angle.value(), targetQubit1);
    break;
  case Gate::SWAP:
    qc.swap(targetQubit1, targetQubit2);
    break;
  case Gate::PhasedRx:
    assert(params.size() == 2 && "PhasedRx gate should have 2 parameters!");
    angle = getConstantDouble(params[0]);
    std::optional<double> phi = getConstantDouble(params[1]);
    qc.r(angle.value(), phi.value(), targetQubit1);
    break;
  }
}

static void loadGateOpsIntoQC(mlir::Operation *gateOp, QuantumOpView qview,
                              qc::QuantumComputation &qc,
                              bool isControlled = false) {

  int64_t controlQubitIdx = -2;
  int64_t targetQubit1Idx = -2;
  int64_t targetQubit2Idx = -2;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;

  if (isControlled) {
    auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
    assert((controlQubitVector.size() == 1) &&
           "Only upto 2-Qubit gates supported!");

    controlQubitIdx = controlQubitVector[0].index;
    if (controlQubitIdx == -1) {
      // Gate operation in Catalyst (value semantics)
      // Qubit in Catalyst
      auto operand = controlQubitVector[0].base;
      controlQubitIdx = getOriginQubit(operand)->index;
    }
  }
  assert((targetQubitVector.size() == 1 || targetQubitVector.size() == 2) &&
         "Only upto 1-Qubit Non-controlled gates supported!");

  targetQubit1Idx = targetQubitVector[0].index;
  if (targetQubitVector.size() == 2) {
    targetQubit2Idx = targetQubitVector[1].index;
  }

  if (targetQubit1Idx == -1) {
    // Gate operation in Catalyst (value semantics)
    // Qubit in Catalyst
    auto operand = targetQubitVector[0].base;
    targetQubit1Idx = getOriginQubit(operand)->index;
  }
  loadGates(gateOp, qc, qview.GateTy, qview.params, controlQubitIdx,
            targetQubit1Idx, targetQubit2Idx);
}
