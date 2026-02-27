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
********************************************************************************
  author Martin Letras
  date   May 2025
  version 1.0
********************************************************************************/
/** @file
  @brief
  @details This header defines a set of functions that are useful to get
  information from MLIR modules. E.g., extract the number of qubits, the
  classical registers. Moreover, it also allows to define variables as MLIR
  constructs.
  @par
  This header must be included to use the available functions to manipulate MLIR
  modules.
*/

#pragma once

#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Support/Plugin.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace mqss_cudaq::support::quakeDialect {

/**
  @brief Function that creates an `mlir::Value` associated to a numeric value.
  @details This functions appends an `mlir::Value` into an MLIR module
 associated to the input `OpBuilder`.
  @param[out] builder is an `OpBuilder` object associated with a MLIR module.
 It is used to insert new instructions to the corresponding MLIR module.
  @param[in] loc is the location of the new inserted instruction.
  @param[in] value is the numeric value to be defined into the MLIR module.
  @return an `mlir::Value` object of the inserted numerical value.
*/
Value createFloatValue(OpBuilder &builder, Location loc, double value);

// TODO: return -1 is not good idea
/**
  @brief Function that extracts a double numeric value from a numeric value in
  an MLIR module.
  @details This functions extracts a `double` from an MLIR `Operation`.
  @param[in] op is the MLIR `Operation` containing a numerical value.
  @return a `double` with the numerical value of op.
*/
double extractDoubleArgumentValue(Operation *op);
// TODO: return -1 is not good idea
/**
  @brief Function that extracts an index of a given `ExtractRefOp` operation.
  @details Given an `ExtractRefOp`, this function extracts the integer of the
  index pointing that reference (qubit index), returns -1 when fail.
  @param[in] op is the MLIR `ExtractRefOp`.
  @return a `int` with the index of the given `ExtractRefOp`.
*/
int64_t extractIndexFromQuakeExtractRefOp(Operation *op);

/**
  @brief Function that get the number of qubits used by a given quantum kernel.
  @details Given a `func::FuncOp` that stores a quantum kernel in Quake. This
  function returns the number of declared qubits within the given quantum
  kernel.
  @param[in] circuit is the input quantum kernel
  @return a `int` with the number of declared qubits.
*/
int getNumberOfQubits(func::FuncOp circuit);

/**
  @brief Function that get the number of classical bits used by a given quantum
  kernel.
  @details Given a `func::FuncOp` that stores a quantum kernel in Quake. This
  function returns the number of declared classical bits within the given
  quantum kernel.
  @param[in] circuit is the input quantum kernel
  @param[out] measurements is a `std::map<int, int>`. This map maps the qubits
  with its corresponding classical bit. The key is the qubit index and the value
  is the classical bit index.
  @return the number of classical bits declared in the given quantum kernel.
*/
int getNumberOfClassicalBits(func::FuncOp circuit,
                             std::map<int, int> &measurements);

/**
  @brief Function that get the number of classical bits used by a given quantum
  kernel.
  @details Given a `func::FuncOp` that stores a quantum kernel in Quake. This
  function returns the number of declared classical bits within the given
  quantum kernel.
  @param[in] circuit is the input quantum kernel
  @return the number of classical bits declared in the given quantum kernel.
*/
int getNumberOfClassicalBits(func::FuncOp circuit);

/**
  @brief Function that get a vector of indices associated with a given
  `mlir::ValueRange`.
  @details Given a `mlir::ValueRange` that stores a list of indices. This
  function converts the `mlir::ValueRange` to a vector of `int`.
  @param[in]  array is the input `mlir::ValueRange`.
  @return a vector of indices stored in the input `mlir::ValueRange` object.
*/
std::vector<int> getIndicesOfValueRange(mlir::ValueRange array);

/**
  @brief Function that get a vector of numerical values associated with a given
  `mlir::ValueRange`.
  @details Given a `mlir::ValueRange` that stores a list of parameters, i.e.,
  rotation angles. This function converts the `mlir::ValueRange` to a vector of
  `double`.
  @param[in]  array is the input `mlir::ValueRange` containing the parameters.
  @return a vector of double stored in the input `mlir::ValueRange` object.
*/
std::vector<double> getParametersValues(mlir::ValueRange array);

/**
  @brief Function get the previous operation on a given target qubit.
  @details Given a `mlir::Operation` and a target qubit. This function get the
  previous operation on the given target qubit, starting from `currentOp`.
  @param[in] currentOp is current quantum gate.
  @param[in] targetQubit is the target qubit to be used as reference.
  @return an mlir::Operation which is the previous operation on the given target
  qubit.
*/
mlir::Operation *getPreviousOperationOnTarget(mlir::Operation *currentOp,
                                              mlir::Value targetQubit);

/**
  @brief Function get the next operation on a given target qubit.
  @details Given a `mlir::Operation` and a target qubit. This function get the
  next operation on the given target qubit, starting from `currentOp`.
  @param[in] currentOp is current quantum gate.
  @param[in] targetQubit is the target qubit to be used as reference.
  @return an mlir::Operation which is the next operation on the given target
  qubit.
*/
mlir::Operation *getNextOperationOnTarget(mlir::Operation *currentOp,
                                          mlir::Value targetQubit);
} // namespace cudaq::support::quakeDialect

namespace supportQuake = mqss_cudaq::support::quakeDialect;
