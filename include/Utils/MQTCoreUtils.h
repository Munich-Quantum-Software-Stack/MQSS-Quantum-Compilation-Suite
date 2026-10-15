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

#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/Location.h>
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

static bool resolveSSAformForMeasureOps(mlir::Operation *OldMeasOp,
                                        SmallVector<mlir::Value> newResults) {
  // llvm::outs() << "Old meas op: " << *OldMeasOp
  //              << " results: " << OldMeasOp->getResults().size() << ","
  //              << newResults.size() << "\n";
  auto OldResults = OldMeasOp->getResults();
  if (OldResults.size() != newResults.size()) {
    mlir::emitError(
        OldMeasOp->getLoc(),
        "ResolveSSA: Old and New MeasureOp should have same Result Qubits!");
    return false;
  }
  for (int j = 0; j < OldResults.size(); ++j) {
    auto oldRes = OldResults[j];
    auto newRes = newResults[j];
    oldRes.replaceAllUsesWith(newRes);
  }
  if (!OldMeasOp->use_empty()) {
    mlir::emitError(OldMeasOp->getLoc(),
                    "ResolveSSA: Old measurement Op should not have any users "
                    "before erasing!");
    return false;
  }
  OldMeasOp->erase();
  return true;
}

// Load the measurement operation within QuantumComputation
static bool loadMeasureOpIntoQC(QuantumOpView qview, qc::QuantumComputation &qc,
                                mlir::Location loc) {

  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  if (targetQubitVector.size() != 1) {
    mlir::emitError(loc,
                    "Mapping: Only Single Qubit Measurement Ops supported");
    return false;
  }

  auto targetQubit = targetQubitVector[0].index;
  for (auto entry : qview.measurements) {
    qc.measure(entry.QubitIndex, entry.ClassicalBitIndex);
  }
  return true;
}

static bool loadGates(mlir::Operation *gateOp, qc::QuantumComputation &qc,
                      Gate GateTy, SmallVector<mlir::Value, 2> params,
                      int64_t controlQubit, int64_t targetQubit1,
                      int64_t targetQubit2 = -2) {

  // llvm::outs() << "Gate Op: " << *gateOp << " , Ty: " << GateTy << "\n";

  std::optional<double> angle;
  std::optional<double> param2;
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
  case Gate::SAdj:
    qc.sdg(targetQubit1);
    break;
  case Gate::T:
    qc.t(targetQubit1);
    break;
  case Gate::TAdj:
    qc.tdg(targetQubit1);
    break;
  case Gate::RX:
    if (params.size() != 1) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: RX gate should have only 1 parameter!");
      return false;
    }
    angle = getConstantDouble(params[0]);
    if (!angle.has_value()) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: No angle found to create RX gate!");
      return false;
    }
    qc.rx(angle.value(), targetQubit1);

    break;
  case Gate::RY:
    if (params.size() != 1) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: RY gate should have only 1 parameter!");
      return false;
    }
    angle = getConstantDouble(params[0]);
    if (!angle.has_value()) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: No angle found to create RY gate!");
      return false;
    }
    qc.ry(angle.value(), targetQubit1);
    break;
  case Gate::RZ:
    if (params.size() != 1) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: RZ gate should have only 1 parameter!");
      return false;
    }
    angle = getConstantDouble(params[0]);
    if (!angle.has_value()) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: No angle found to create RZ gate!");
      return false;
    }
    qc.rz(angle.value(), targetQubit1);
    break;
  case Gate::SWAP:
    qc.swap(targetQubit1, targetQubit2);
    break;
  case Gate::PhasedRx:
    if (params.size() != 2) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: PhasedRx gate should have 2 parameters!");
      return false;
    }
    angle = getConstantDouble(params[0]);
    param2 = getConstantDouble(params[1]);
    if (!angle.has_value() || !param2.has_value()) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: Missing angle needed to create PhasedRx gate!");
      return false;
    }

    qc.r(angle.value(), param2.value(), targetQubit1);
    break;
  default:
    mlir::emitError(gateOp->getLoc(), "Mapping: Unsupported Gate Type");
    return false;
    break;
  }
  return true;
}

static bool loadGateOpsIntoQC(mlir::Operation *gateOp, QuantumOpView qview,
                              qc::QuantumComputation &qc,
                              bool isControlled = false) {

  int64_t controlQubitIdx = -2;
  int64_t targetQubit1Idx = -2;
  int64_t targetQubit2Idx = -2;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;

  if (isControlled) {
    auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
    if ((controlQubitVector.size() != 1)) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: Gates with 2 Control Qubits not supported");
      return false;
    }

    controlQubitIdx = controlQubitVector[0].index;
    if (controlQubitIdx == -1) {
      // Gate operation in Catalyst (value semantics)
      // Qubit in Catalyst
      auto operand = controlQubitVector[0].base;
      auto controlOrig = getOriginQubit(operand);
      if (controlOrig.has_value()) {
        controlQubitIdx = controlOrig->index;
      } else {
        mlir::emitError(operand.getLoc(),
                        "Mapping: Could not fetch origin Qubit of operand!");
        return false;
      }
    }
  }
  if (targetQubitVector.size() != 1 && targetQubitVector.size() != 2) {
    mlir::emitError(gateOp->getLoc(),
                    "Mapping: Gates with 1 or 2 target Qubits supported!");
    return false;
  }

  targetQubit1Idx = targetQubitVector[0].index;
  if (targetQubitVector.size() == 2) {
    targetQubit2Idx = targetQubitVector[1].index;
  }

  if (targetQubit1Idx == -1) {
    // Gate operation in Catalyst (value semantics)
    // Qubit in Catalyst
    auto operand = targetQubitVector[0].base;
    auto targetOrig = getOriginQubit(operand);
    if (targetOrig.has_value()) {
      targetQubit1Idx = targetOrig->index;
    } else {
      mlir::emitError(operand.getLoc(),
                      "Mapping: Could not fetch origin Qubit of operand!");
      return false;
    }
  }
  return loadGates(gateOp, qc, qview.GateTy, qview.params, controlQubitIdx,
                   targetQubit1Idx, targetQubit2Idx);
}
