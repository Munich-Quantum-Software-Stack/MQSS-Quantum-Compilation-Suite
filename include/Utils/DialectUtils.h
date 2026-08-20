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

#pragma once

using namespace mlir;
using namespace llvm;
using namespace cudaq;

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

inline quake::OperatorInterface
createQuakeGate(Location loc, llvm::StringRef NewGateTy,
                const SmallVector<mlir::Value, 2> ControlQubits,
                const SmallVector<mlir::Value, 2> TargetQubitOps,
                const mlir::ValueRange params, mlir::IRRewriter &builder,
                bool isAdj = false) {

  if (NewGateTy == "RX") {
    // TODO: Can this create an gate?
    return builder.create<quake::RxOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RY") {
    // TODO: Can this create an gate?
    return builder.create<quake::RyOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RZ") {
    // TODO: Can this create an gate?
    return builder.create<quake::RzOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "S") {
    return builder.create<quake::SOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "T") {
    return builder.create<quake::TOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "SAdj") {
    return builder.create<quake::SOp>(loc, true, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "TAdj") {
    return builder.create<quake::TOp>(loc, true, params, ControlQubits,
                                      TargetQubitOps);
  }

  if (NewGateTy == "PauliZ") {
    return builder.create<quake::ZOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliX") {
    return builder.create<quake::XOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliY") {
    return builder.create<quake::YOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "H") {
    return builder.create<quake::HOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "CNOT" || NewGateTy == "CX") {
    return builder.create<quake::XOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CY") {
    return builder.create<quake::YOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CZ") {
    return builder.create<quake::ZOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "SWAP") {
    return builder.create<quake::SwapOp>(loc, params, ControlQubits,
                                         TargetQubitOps);
  }
  return nullptr;
}

inline mlir::arith::ConstantOp
createConstant(Location loc, mlir::IRRewriter &builder, TypedAttr attr) {

  // arith::ConstantOp with FloatAttr works on both LLVM-16 and LLVM-21
  return builder.create<mlir::arith::ConstantOp>(loc, attr);
}

inline mlir::arith::ConstantOp createQuakeConstOp(Location loc,
                                                  mlir::IRRewriter &builder,
                                                  double constantValue,
                                                  mlir::Type type) {

  if (isa<FloatType>(type)) {
    auto valueAttr = builder.getFloatAttr(type, constantValue);
    return createConstant(loc, builder, valueAttr);
  }
}

inline mlir::Value createQuakeDivF(Location loc, mlir::Value numerator,
                                   double denominator,
                                   mlir::IRRewriter &rewriter) {
  auto denominatorValue = rewriter.create<arith::ConstantOp>(
      loc, rewriter.getF64FloatAttr(denominator));
  return rewriter.create<arith::DivFOp>(loc, numerator, denominatorValue);
}

inline quake::AllocaOp
createQuakeAlloca(Location loc, mlir::IRRewriter &builder, size_t numQubits) {

  return builder.create<quake::AllocaOp>(
      loc, quake::VeqType::get(builder.getContext(), numQubits));
}

inline quake::ExtractRefOp createQuakeExtractRefOp(Location loc,
                                                   mlir::IRRewriter &builder,
                                                   mlir::Value qubits,
                                                   unsigned int targetQubit) {

  return builder.create<quake::ExtractRefOp>(loc, qubits, targetQubit);
}

inline SmallVector<mlir::Value, 2>
createQuakeMeasureOp(Location loc, mlir::IRRewriter &builder,
                     const SmallVector<mlir::Value, 2> TargetQubits) {

  SmallVector<mlir::Value, 2> results;
  mlir::Type measTy = quake::MeasureType::get(builder.getContext());
  auto newOp = builder.create<quake::MzOp>(loc, measTy, TargetQubits);

  for (auto res : newOp->getResults()) {
    results.push_back(res);
  }

  return results;
}

using namespace mlir;
using namespace catalyst;

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

inline quantum::CustomOp
createCatalystGate(Location loc, llvm::StringRef NewGateTy,
                   const SmallVector<mlir::Value, 2> ControlQubitsOps,
                   const SmallVector<mlir::Value, 2> TargetQubitOps,
                   const mlir::ValueRange params, mlir::IRRewriter &builder,
                   bool isAdj = false) {

  quantum::CustomOp NewOp = nullptr;
  if (ControlQubitsOps.empty()) {
    std::vector<mlir::Type> TargetQubitTys;

    for (auto t : TargetQubitOps) {
      TargetQubitTys.push_back(t.getType());
    }
    if (NewGateTy == "SAdj") {
      NewGateTy = "S";
      isAdj = true;
    }
    if (NewGateTy == "TAdj") {
      NewGateTy = "T";
      isAdj = true;
    }

    return builder.create<quantum::CustomOp>(
        loc,
        /*out_qubits=*/mlir::TypeRange(TargetQubitTys),
        /*out_ctrl_qubits=*/mlir::TypeRange(),
        /*params=*/params,
        /*in_qubits=*/mlir::ValueRange(TargetQubitOps),
        /*gate_name=*/NewGateTy,
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange(),
        /*in_ctrl_values=*/mlir::ValueRange());
    // TODO: Revisit this
    // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  }

  StringAttr GateTy;
  if (NewGateTy == "CNOT") {
    GateTy = builder.getStringAttr("CNOT");
  }
  if (NewGateTy == "CZ") {
    GateTy = builder.getStringAttr("CZ");
  }

  mlir::Value control = ControlQubitsOps[0];
  mlir::Value target = TargetQubitOps[0];

  return builder.create<quantum::CustomOp>(
      loc,
      /*out_qubits=*/mlir::TypeRange{control.getType(), target.getType()},
      /*out_ctrl_qubits=*/mlir::TypeRange{},
      /*params=*/mlir::ValueRange{},
      /*in_qubits=*/mlir::ValueRange{control, target},
      /*gate_name=*/GateTy, // or gateName if API
                            // accepts StringRef
      /*adjoint=*/isAdj,
      /*in_ctrl_qubits=*/mlir::ValueRange{},
      /*in_ctrl_values=*/mlir::ValueRange{});
  // TODO: Revisit this
  // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  // OpToReplace->getResult(1).replaceAllUsesWith(NewOp->getResult(1));
}

inline mlir::arith::ConstantOp createCatalystConstOp(Location loc,
                                                     mlir::IRRewriter &builder,
                                                     double constantValue,
                                                     mlir::Type type) {

  if (isa<FloatType>(type)) {
    auto valueAttr = builder.getFloatAttr(type, constantValue);
    return createConstant(loc, builder, valueAttr);
  }
}

inline mlir::Value createCatalystDivF(Location loc, mlir::Value numerator,
                                      double denominator,
                                      mlir::IRRewriter &rewriter) {
  auto denominatorValue = rewriter.create<arith::ConstantOp>(
      loc, rewriter.getF64FloatAttr(denominator));
  return rewriter.create<arith::DivFOp>(loc, numerator, denominatorValue);
}

inline quantum::AllocOp createCatalystAlloca(Location loc,
                                             mlir::IRRewriter &builder,
                                             size_t numQubits) {

  auto regTy = quantum::ResultType::get(builder.getContext());
  mlir::Type qregTy = mlir::parseType("!quantum.reg", builder.getContext());
  assert(qregTy && "failed to parse !quantum.reg");

  return builder.create<quantum::AllocOp>(
      loc, qregTy,
      /*nqubits=*/mlir::Value{},
      /*nqubits_attr=*/builder.getI64IntegerAttr(numQubits));
}

inline quantum::ExtractOp createCatalystExtractRefOp(Location loc,
                                                     mlir::IRRewriter &builder,
                                                     mlir::Value allocQubits,
                                                     unsigned int targetQubit) {

  mlir::Type qbitTy = mlir::parseType("!quantum.bit", builder.getContext());

  assert(qbitTy && "failed to parse !quantum.bit");

  auto indexAttr = builder.getI64IntegerAttr(targetQubit);

  return builder.create<quantum::ExtractOp>(loc, qbitTy, allocQubits,
                                            /*index=*/mlir::Value{},
                                            /*index_attr=*/indexAttr);
}

inline SmallVector<mlir::Value, 2>
createCatalystMeasureOp(mlir::Location loc, mlir::IRRewriter &builder,
                        const SmallVector<mlir::Value, 2> TargetQubitOps) {

  SmallVector<mlir::Value> results;
  auto i1Ty = builder.getI1Type();
  for (mlir::Value q : TargetQubitOps) {
    auto qbitTy = q.getType(); // should be !quantum.bit
    auto m =
        builder.create<quantum::MeasureOp>(loc, TypeRange{i1Ty, qbitTy}, q);
    // measured classical result
    auto measResult = m.getMres();
    // updated SSA qubit
    auto outQubit = m.getOutQubit();

    results.push_back(measResult);
    results.push_back(outQubit);
  }
  // // Access results
  // mlir::Value outQubit = measureOp.getOutQubit(); // !quantum.bit
  // mlir::Value mres     = measureOp.getMres();     // i1
  return results;
}
