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

#pragma once

#include "Utils/DialectUtils.h"
#include "Utils/Error.h"
#include "Utils/TransformUtils.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

#include "llvm/IR/Module.h"

#include <cmath>
#include <cudaq/Optimizer/Dialect/Quake/QuakeOps.h>
#include <llvm/ADT/StringExtras.h>
#include <mlir/IR/PatternMatch.h>
#include <mlir/IR/Value.h>
#include <mlir/IR/ValueRange.h>

// IQM
// Supported exchange formats:
//  - QIRBASESTRING
//  - IQMJSON
// Native gate set:
//  - cz
//  - measure
//  - prx
//  - prx_12

// PLANQC
// Supported exchange formats:
//  - QASM2
//  - QASM3
// Native gate-set:
//  - rx
//  - cz
//  - rz
//  - measure
//  - swap

// WMI
// Supported Exchange formats:
//  - QIRBASESTRING
//  - QIRBASEMODULE
// Native gate-set:
//  - cz
//  - id
//  - x
//  - y
//  - sx
//  - rz
//  - mz

//===----------------------------------------------------------------------===//
// Rewrite rules.
//
// Each function below reproduces, verbatim, the rewrite performed by the
// corresponding single-purpose pass in this directory (e.g. rewriteXToRx()
// mirrors XToRx.cpp). They are kept separate from those passes rather than
// calling into them because each one only walks/matches its own op; here we
// already know which rule matched a given op and just need to apply it.
//===----------------------------------------------------------------------===//

// The Rx here is always a fixed pi/2 rotation ("sx"/sqrt-X), never a
// parameterized one -- BasisConversionPass's classifyOp() relies on that to
// tell this apart from a genuinely arbitrary-angle Rx.
bool rewriteHToRzRxRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                      quake::HOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);

  auto c1 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!c1)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {c1}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RX", {}, {target}, {c1}, rewriter);
  if (!newGate)
    return false;
  auto c2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!c2)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", {}, {target}, {c2}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteHToU3(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::HOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto halfPi = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  auto pi = createArithConstantOp(loc, DialectTy, rewriter, M_PI);
  if (!halfPi || !zero || !pi)
    return false;
  auto *newGate = createNewGate(loc, DialectTy, "U3", {}, {target},
                                {halfPi, zero, pi}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteXToHZH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                   quake::XOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "PauliZ", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteXToRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::XOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, M_PI);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RX", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteYToRy(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::YOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, -M_PI);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RY", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteZToHXH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                   quake::ZOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "PauliX", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteZToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::ZOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, M_PI);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::SOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSToSdgSdgSdg(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                         quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "SAdj", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "SAdj", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "SAdj", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSToTT(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "T", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "T", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSdgToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                    quake::SOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, -M_PI_2);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSdgToSSS(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                     quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "S", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "S", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "S", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteTToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  quake::TOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, M_PI_4);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteTdgToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                    quake::TOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createArithConstantOp(loc, DialectTy, rewriter, -M_PI_4);
  if (!angle)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {angle}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteR1ToRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                   quake::R1Op op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {param}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteRxToHRzH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                     quake::RxOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);

  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {param}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

// TODO: Is this correct?
//       c1 and c2 are both Pi/2
bool rewriteRyToRzRxRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::RyOp op) {
  Location loc = op.getLoc();
  auto rotation = op.getParameters()[0];
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto c1 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!c1)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {c1}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RX", {}, {target}, {rotation}, rewriter);
  if (!newGate)
    return false;
  auto c2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!c2)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", {}, {target}, {c2}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteRzToHRxH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                     quake::RzOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RX", {}, {target}, {param}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteRzToU3(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                   quake::RzOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  if (!zero)
    return false;
  auto *newGate = createNewGate(loc, DialectTy, "U3", {}, {target},
                                {zero, zero, param}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteU2ToRzRyRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::U2Op op) {
  auto params = op.getParameters();
  auto phi = params[0];
  auto lambda = params[1];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto halfPi = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);

  if (!halfPi)
    return false;
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {phi}, rewriter);

  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RY", {}, {target}, {halfPi}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {lambda}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteU2ToU3(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                   quake::U2Op op) {
  auto params = op.getParameters();
  auto phi = params[0];
  auto lambda = params[1];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto halfPi = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!halfPi)
    return false;
  auto *newGate = createNewGate(loc, DialectTy, "U3", {}, {target},
                                {halfPi, phi, lambda}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteU3ToRzRyRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::U3Op op) {
  auto params = op.getParameters();
  auto theta = params[0];
  auto phi = params[1];
  auto lambda = params[2];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {lambda}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "RY", {}, {target}, {theta}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", {}, {target}, {phi}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSwapToUpperCxCxCx(const QuantumDialect &DialectTy,
                              IRRewriter &rewriter, quake::SwapOp op) {
  auto q0 = op.getTargets()[0];
  auto q1 = op.getTargets()[1];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate = createNewGate(loc, DialectTy, "CX", {q0}, {q1}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "CX", {q1}, {q0}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "CX", {q0}, {q1}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCxToUpperHCzH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                          quake::XOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);

  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "CZ", {control}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCzToUpperHCxH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                          quake::ZOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "CX", {control}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCyToSCxSdg(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::YOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "S", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "CX", {control}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "SAdj", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCrxToHCrzH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::RxOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", {control}, {target}, {param},
                          rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCrzToHCrxH(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto *newGate =
      createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RX", {control}, {target}, {param},
                          rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "H", {}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCrzToRzCu3(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                       quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);

  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  auto halfAngle = createDivFOp(loc, DialectTy, angle, 2.0, rewriter);
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  if (!halfAngle || !zero)
    return false;
  auto negHalfAngle = arith::NegFOp::create(rewriter, loc, halfAngle);
  auto *newGate = createNewGate(loc, DialectTy, "RZ", {control}, {},
                                ValueRange{negHalfAngle}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "U3", {control}, {target},
                          {zero, zero, param}, rewriter);
  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCrzToRzCxRzCx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                          quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  mlir::Value halfAngle = createDivFOp(loc, DialectTy, angle, 2.0, rewriter);
  if (!halfAngle)
    return false;
  mlir::Value negHalfAngle = arith::NegFOp::create(rewriter, loc, halfAngle);

  auto *newGate =
      createNewGate(loc, DialectTy, "RZ", {}, {target}, {halfAngle}, rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "CX", {control}, {target}, {}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", {}, {target}, {negHalfAngle},
                          rewriter);
  if (!newGate)
    return false;
  newGate =
      createNewGate(loc, DialectTy, "CX", {control}, {target}, {}, rewriter);

  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteCryToRzCrxRz(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                         quake::RyOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto negHalfPi = createArithConstantOp(loc, DialectTy, rewriter, -M_PI_2);
  auto halfPi = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);

  if (!negHalfPi || !halfPi)
    return false;
  auto *newGate = createNewGate(loc, DialectTy, "RZ", ValueRange{}, {target},
                                ValueRange{negHalfPi}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RX", {control}, {target},
                          ValueRange{param}, rewriter);
  if (!newGate)
    return false;
  newGate = createNewGate(loc, DialectTy, "RZ", ValueRange{}, {target},
                          ValueRange{halfPi}, rewriter);

  if (!newGate)
    return false;
  rewriter.eraseOp(op);
  return true;
}

bool rewriteHToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                        quake::HOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  rewriter.setInsertionPointAfter(op);
  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  auto pi = createArithConstantOp(loc, DialectTy, rewriter, M_PI);
  auto pi_2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);

  if (!zero || !pi || !pi_2)
    return false;
  std::array<mlir::Value, 2> parameters = {pi_2, pi_2};
  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate)
    return false;
  parameters[0] = pi;
  parameters[1] = zero;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate)
    return false;

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
  return true;
}

bool rewriteXToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                        cudaq::quake::XOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  rewriter.setInsertionPointAfter(op);
  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  auto pi = createArithConstantOp(loc, DialectTy, rewriter, M_PI);

  if (!zero || !pi)
    return false;
  SmallVector<mlir::Value> parameters = {pi, zero};
  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate)
    return false;
  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
  return true;
}

bool rewriteRxToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                         cudaq::quake::RxOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle = op.getParameter();
  rewriter.setInsertionPointAfter(op);
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  mlir::Type angleType = op.getParameter().getType();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  mlir::Value zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);

  if (!zero)
    return false;
  SmallVector<mlir::Value> parameters = {angle, zero};
  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);

  if (!newGate)
    return false;
  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
  return true;
}

bool rewriteRzToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                         cudaq::quake::RzOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle = op.getParameter();
  rewriter.setInsertionPointAfter(op);
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  mlir::Type angleType = op.getParameter().getType();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  auto pi_2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!zero || !pi_2)
    return false;

  auto negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);
  auto negAngle = arith::NegFOp::create(rewriter, loc, angle);

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate)
    return false;
  parameters[0] = negAngle;
  parameters[1] = pi_2;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate)
    return false;
  parameters[0] = negPi_2;
  parameters[1] = zero;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate)
    return false;

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
  return true;
}

bool rewriteR1ToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                         cudaq::quake::R1Op op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  auto angleType = op.getParameter().getType();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  auto pi_2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!zero || !pi_2)
    return false;
  auto negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);
  auto negAngle = arith::NegFOp::create(rewriter, loc, angle);

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = negAngle;
  parameters[1] = pi_2;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = negPi_2;
  parameters[1] = zero;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);

  if (!newGate) {
    return false;
  }
  rewriter.eraseOp(op);
  return true;
}

bool rewriteYToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                        cudaq::quake::YOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  // Necessary/Helpful constants
  auto pi = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  auto negPi_2 = createArithConstantOp(loc, DialectTy, rewriter, -M_PI_2);

  if (!pi || !negPi_2) {
    return false;
  }

  SmallVector<mlir::Value> parameters = {pi, negPi_2};
  SmallVector<mlir::Value> noControls;
  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate) {
    return false;
  }
  rewriter.eraseOp(op);
  return true;
}

bool rewriteSToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                        cudaq::quake::SOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  mlir::Value zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  mlir::Value pi_2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);
  if (!zero || !pi_2)
    return false;
  mlir::Value negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);

  mlir::Value angle = op.isAdj() ? pi_2 : negPi_2;

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = angle;
  parameters[1] = pi_2;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = negPi_2;
  parameters[1] = zero;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate) {
    return false;
  }
  rewriter.eraseOp(op);
  return true;
}

bool rewriteTToPhasedRx(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                        cudaq::quake::TOp op) {
  if (!op.getControls().empty())
    return false;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle = createArithConstantOp(loc, DialectTy, rewriter, -M_PI_4);
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createArithConstantOp(loc, DialectTy, rewriter, 0.0);
  mlir::Value pi_2 = createArithConstantOp(loc, DialectTy, rewriter, M_PI_2);

  if (!angle || !zero || !pi_2)
    return false;
  mlir::Value negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);
  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  auto *newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls,
                                {target}, parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = angle;
  parameters[1] = pi_2;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);
  if (!newGate) {
    return false;
  }
  parameters[0] = negPi_2;
  parameters[1] = zero;
  newGate = createNewGate(loc, DialectTy, "PhasedRx", noControls, {target},
                          parameters, rewriter);

  if (!newGate) {
    return false;
  }

  rewriter.eraseOp(op);
  return true;
}

//===----------------------------------------------------------------------===//
// The rule table: for every recognized "native gate mnemonic", the list of
// known ways to build it out of other gates. Multiple entries mean multiple
// choices exist in lib/Passes/Decompositions/ for that gate.
//===----------------------------------------------------------------------===//

struct DecompositionRule {
  const char *name;
  std::vector<std::string> produces;
  std::function<bool(const QuantumDialect &, IRRewriter &, Operation *)> apply;
};

// A named functor, rather than a lambda defined inside makeRule's body: a
// lambda there is a *local type*, and some GCC versions mishandle
// constructing std::function from it once the same makeRule<OpT>
// instantiation is used more than once (e.g. makeRule<quake::R1Op> for both
// rewriteR1ToRz and rewriteR1ToPhasedRx) -- `-fpermissive` lets it past the
// local-type-as-template-argument rule but then fails to emit a definition
// for that constructor instantiation ("used but never defined"). A functor
// at namespace scope has ordinary external linkage, so this doesn't apply.
template <typename OpT> struct RuleAdaptor {
  bool (*rewrite)(const QuantumDialect &, IRRewriter &, OpT);

  bool operator()(const QuantumDialect &DialectTy, IRRewriter &rewriter,
                  Operation *op) const {
    return rewrite(DialectTy, rewriter, cast<OpT>(op));
  }
};

template <typename OpT>
DecompositionRule makeRule(const char *name, std::vector<std::string> produces,
                           bool (*rewrite)(const QuantumDialect &, IRRewriter &,
                                           OpT)) {
  return DecompositionRule{name, std::move(produces),
                           RuleAdaptor<OpT>{rewrite}};
}
