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

#include "Quantum/IR/QuantumOps.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

#include <llvm/Support/Casting.h>
#include <mlir/IR/Attributes.h>
#include <mlir/IR/BuiltinTypeInterfaces.h>
#include <mlir/IR/Types.h>
#include <mlir/IR/Value.h>
#include <vector>

#pragma once

using namespace mlir;
using namespace llvm;
using namespace cudaq;
using namespace catalyst;

// Maps a Quake op to the name of the Gate it represents. An op is only
// reported as a gate if its *control count matches that gate*: 0 or exactly 1
// control for the gates that have a controlled variant (X/Y/Z/Rx/Ry/Rz/H ->
// CNOT/CY/CZ/CRX/CRY/CRZ/CH), and 0 controls for the rest. Anything else (a
// Toffoli, a controlled S, ...) is reported as {false, ""} so consumers see
// GateTy == UNKNOWN instead of a mislabelled gate.
//
// Target and parameter counts are deliberately NOT checked here: they are
// already enforced by the dialect's NumTargets<N>/NumParameters<N> op traits
// (QuakeOps.td), so a verified module can't violate them.
//
// Adjointness is not part of the shape and is not checked here either, except
// where it is part of the gate's name (SAdj/TAdj); consumers read it from
// QuantumOpView::isAdjoint.
inline std::tuple<bool, StringLiteral> isQuakeQuantumGate(Operation *op) {
  using Result = std::tuple<bool, StringLiteral>;
  const Result NotAGate{false, ""};

  // 0 controls -> `plain`, exactly 1 control -> `controlled`, else not a gate.
  auto zeroOrOneControl = [&](size_t numControls, StringLiteral plain,
                              StringLiteral controlled) -> Result {
    if (numControls == 0)
      return {true, plain};
    if (numControls == 1)
      return {true, controlled};
    return NotAGate;
  };
  // Gates that only exist without controls.
  auto uncontrolled = [&](size_t numControls, StringLiteral name) -> Result {
    if (numControls == 0)
      return {true, name};
    return NotAGate;
  };

  if (auto x = dyn_cast<quake::XOp>(op))
    return zeroOrOneControl(x.getControls().size(), "PauliX", "CNOT");
  if (auto x = dyn_cast<quake::YOp>(op))
    return zeroOrOneControl(x.getControls().size(), "PauliY", "CY");
  if (auto x = dyn_cast<quake::ZOp>(op))
    return zeroOrOneControl(x.getControls().size(), "PauliZ", "CZ");
  if (auto x = dyn_cast<quake::HOp>(op))
    return zeroOrOneControl(x.getControls().size(), "H", "CH");

  if (auto x = dyn_cast<quake::RxOp>(op))
    return zeroOrOneControl(x.getControls().size(), "RX", "CRX");
  if (auto x = dyn_cast<quake::RyOp>(op))
    return zeroOrOneControl(x.getControls().size(), "RY", "CRY");
  if (auto x = dyn_cast<quake::RzOp>(op))
    return zeroOrOneControl(x.getControls().size(), "RZ", "CRZ");

  if (auto x = dyn_cast<quake::R1Op>(op))
    return uncontrolled(x.getControls().size(), "R1");
  if (auto x = dyn_cast<quake::PhasedRxOp>(op))
    return uncontrolled(x.getControls().size(), "PhasedRx");
  if (auto x = dyn_cast<quake::SwapOp>(op))
    return uncontrolled(x.getControls().size(), "SWAP");
  if (auto x = dyn_cast<quake::U2Op>(op))
    return uncontrolled(x.getControls().size(), "U2");
  if (auto x = dyn_cast<quake::U3Op>(op))
    return uncontrolled(x.getControls().size(), "U3");

  if (auto x = dyn_cast<quake::SOp>(op))
    return uncontrolled(x.getControls().size(),
                        x.isAdj() ? StringLiteral("SAdj") : StringLiteral("S"));
  if (auto x = dyn_cast<quake::TOp>(op))
    return uncontrolled(x.getControls().size(),
                        x.isAdj() ? StringLiteral("TAdj") : StringLiteral("T"));

  return NotAGate;
}

inline quantum::CustomOp isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<quantum::CustomOp>(op)) {
    return g;
  }
  return nullptr;
}

inline bool hasQuantumEffect(Operation *op) {
  for (auto type : op->getOperandTypes()) {
    if (isa<quantum::QubitType>(type))
      return true;
  }
  for (auto type : op->getResultTypes()) {
    if (isa<quantum::QubitType>(type))
      return true;
  }
  return false;
}
