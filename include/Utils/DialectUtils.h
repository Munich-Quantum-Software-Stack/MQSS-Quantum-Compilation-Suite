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

inline std::tuple<bool, StringLiteral> isQuakeQuantumGate(Operation *op) {

  if (auto x = dyn_cast<quake::XOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliX"};
    return {true, "CNOT"};
  }

  if (auto x = dyn_cast<quake::YOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliY"};
    return {true, "CY"};
  }
  if (auto x = dyn_cast<quake::ZOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliZ"};
    return {true, "CZ"};
  }

  if (auto x = dyn_cast<quake::RxOp>(op))
    return {true, "RX"};
  if (auto x = dyn_cast<quake::RyOp>(op))
    return {true, "RY"};
  if (auto x = dyn_cast<quake::RzOp>(op))
    return {true, "RZ"};

  if (auto x = dyn_cast<quake::HOp>(op)) {
    if (!x.getControls().empty())
      return {true, "CH"};
    return {true, "H"};
  }
  if (auto x = dyn_cast<quake::PhasedRxOp>(op)) {
    return {true, "PhasedRx"};
  }
  if (auto x = dyn_cast<quake::SwapOp>(op)) {
    return {true, "SWAP"};
  }

  if (auto x = dyn_cast<quake::SOp>(op)) {
    if (x.isAdj())
      return {true, "SAdj"};
    return {true, "S"};
  }
  if (auto t = dyn_cast<quake::TOp>(op)) {
    if (t.isAdj())
      return {true, "TAdj"};
    return {true, "T"};
  }
  return {false, ""};
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
