/* This code and any associated documentation is provided "as is"

Copyright 2025 Munich Quantum Software Stack Project

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
  date   February 2025
  version 1.0
  brief
    Set of the most functions used to manipulate Quake MLIR modules. E.g.,
    contains functions to define integer, and double constants, extract
    references, get the number of qubits, classical registers, etc.

*******************************************************************************
* This source code and the accompanying materials are made available under    *
* the terms of the Apache License 2.0 which accompanies this distribution.    *
******************************************************************************/

#include "Utils/Quake.hpp"

// Given a OpBuilder and a double value, it inserts a double in the mlir
// module pointer by the OpBuilder and returns the inserted Value
Value mqss_cudaq::support::quakeDialect::createFloatValue(OpBuilder &builder,
                                                    Location loc,
                                                    double value) {
  // Create a constant value (20.0 of type f64)
  auto valueAttr = builder.getFloatAttr(builder.getF64Type(), value);
  auto constantOp = builder.create<mlir::arith::ConstantOp>(loc, valueAttr);
  return constantOp.getResult();
}

// TODO: return -1 is not good idea
// Given an argument value as Operation, it extracts a double, it the operation
// is not double, returns -1.0 when fail
double mqss_cudaq::support::quakeDialect::extractDoubleArgumentValue(Operation *op) {
  if (auto constantOp = dyn_cast<mlir::arith::ConstantOp>(op))
    if (auto floatAttr = constantOp.getValue().dyn_cast<mlir::FloatAttr>())
      return static_cast<float>(floatAttr.getValueAsDouble());
  return -1.0;
}
// TODO: return -1 is not good idea
// Given an ExtractRefOp, it extracts the integer of the index pointing that
// reference (qubit index), returns -1 when fail
int64_t
mqss_cudaq::support::quakeDialect::extractIndexFromQuakeExtractRefOp(Operation *op) {
  if (auto extractRefOp = llvm::dyn_cast<quake::ExtractRefOp>(op)) {
    auto rawIndexAttr =
        extractRefOp->getAttrOfType<mlir::IntegerAttr>("rawIndex");
    return rawIndexAttr.getInt();
  }
  return -1;
}

// function to get the number of qubits in a given quantum kernel
int mqss_cudaq::support::quakeDialect::getNumberOfQubits(func::FuncOp circuit) {
  int numQubits = 0;
  circuit.walk([&](quake::AllocaOp allocOp) {
    if (auto qrefType = allocOp.getType().dyn_cast<quake::RefType>()) {
      numQubits += 1;
    } else if (auto qvecType = allocOp.getType().dyn_cast<quake::VeqType>()) {
      numQubits += qvecType.getSize();
    }
  });
  return numQubits;
}

// Function to get the number of classical bits allocated in a given
// quantum kernel, it also stores information of the qiubit position
int mqss_cudaq::support::quakeDialect::getNumberOfClassicalBits(
    func::FuncOp circuit, std::map<int, int> &measurements) {
  int numBits = 0;
  circuit.walk([&](mlir::Operation *op) {
    if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op)) {
      for (auto operand : op->getOperands()) {
        if (operand.getType()
                .isa<quake::RefType>()) { // Check if it's qubit reference
          int qubitIndex =
              extractIndexFromQuakeExtractRefOp(operand.getDefiningOp());
          assert(qubitIndex != -1 && "Non valid qubit index for measurement!");
          measurements[qubitIndex] = numBits;
          numBits += 1;
        } else if (operand.getType().isa<quake::VeqType>()) {
          auto qvecType = operand.getType().dyn_cast<quake::VeqType>();
          numBits += qvecType.getSize();
          for (int i = 0; i < numBits; i++) {
            measurements[i] = i;
          }
        }
      }
    }
  });
  return numBits;
}

// Function to get the number of classical bits allocated in
// a given quantum kernel
int mqss_cudaq::support::quakeDialect::getNumberOfClassicalBits(
    func::FuncOp circuit) {
  int numBits = 0;
  circuit.walk([&](mlir::Operation *op) {
    if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op)) {
      for (auto operand : op->getOperands()) {
        if (operand.getType()
                .isa<quake::RefType>()) { // Check if it's a qubit reference
          int qubitIndex =
              extractIndexFromQuakeExtractRefOp(operand.getDefiningOp());
          assert(qubitIndex != -1 && "Non valid qubit index for measurement!");
          numBits += 1;
        } else if (operand.getType().isa<quake::VeqType>()) {
          auto qvecType = operand.getType().dyn_cast<quake::VeqType>();
          numBits += qvecType.getSize();
        }
      }
    }
  });
  return numBits;
}

// Function that get the indices of the Value objectes in array
std::vector<int>
mqss_cudaq::support::quakeDialect::getIndicesOfValueRange(mlir::ValueRange array) {
  std::vector<int> indices;
  for (auto value : array) {
    int qubit_index = extractIndexFromQuakeExtractRefOp(value.getDefiningOp());
    indices.push_back(qubit_index);
  }
  return indices;
}

// At the moment, it is assumed that the parameters are of type Double
std::vector<double>
mqss_cudaq::support::quakeDialect::getParametersValues(mlir::ValueRange array) {
  std::vector<double> parameters;
  for (auto value : array) {
    double param = extractDoubleArgumentValue(value.getDefiningOp());
    parameters.push_back(param);
  }
  return parameters;
}

// Get the previous operation on a given TargeQubit, starting from
// currentOp
mlir::Operation *mqss_cudaq::support::quakeDialect::getPreviousOperationOnTarget(
    mlir::Operation *currentOp, mlir::Value targetQubit) {
  // Start from the previous operation
  mlir::Operation *prevOp = currentOp->getPrevNode();
  // Iterate through the previous operations in the block
  while (prevOp) {
    // Check if the operation has a target qubit and matches the given target
    if (auto quakeOp = dyn_cast<quake::OperatorInterface>(prevOp)) {
      int targetQCurr =
          extractIndexFromQuakeExtractRefOp(targetQubit.getDefiningOp());
      for (mlir::Value target : quakeOp.getTargets()) {
        int targetQPrev =
            extractIndexFromQuakeExtractRefOp(target.getDefiningOp());
        if (targetQCurr == targetQPrev)
          return prevOp;
      }
      for (mlir::Value control : quakeOp.getControls()) {
        int controlQPrev =
            extractIndexFromQuakeExtractRefOp(control.getDefiningOp());
        if (targetQCurr == controlQPrev)
          return prevOp;
      }
    }
    // Move to the previous operation
    prevOp = prevOp->getPrevNode();
  }
  return nullptr; // No matching previous operation found
}

// Get the next operation on a given TargeQubit, starting from
// currentOp
mlir::Operation *mqss_cudaq::support::quakeDialect::getNextOperationOnTarget(
    mlir::Operation *currentOp, mlir::Value targetQubit) {
  // Start from the next operation
  mlir::Operation *nextOp = currentOp->getNextNode();
  // Iterate through the previous operations in the block
  while (nextOp) {
    // Check if the operation has a target qubit and matches the given target
    if (auto quakeOp = dyn_cast<quake::OperatorInterface>(nextOp)) {
      int targetQCurr =
          extractIndexFromQuakeExtractRefOp(targetQubit.getDefiningOp());
      for (mlir::Value target : quakeOp.getTargets()) {
        int targetQNext =
            extractIndexFromQuakeExtractRefOp(target.getDefiningOp());
        if (targetQCurr == targetQNext)
          return nextOp;
      }
      for (mlir::Value control : quakeOp.getControls()) {
        int controlQNext =
            extractIndexFromQuakeExtractRefOp(control.getDefiningOp());
        if (targetQCurr == controlQNext)
          return nextOp;
      }
    }
    // Move to the previous operation
    nextOp = nextOp->getNextNode();
  }
  return nullptr; // No matching previous operation found
}
