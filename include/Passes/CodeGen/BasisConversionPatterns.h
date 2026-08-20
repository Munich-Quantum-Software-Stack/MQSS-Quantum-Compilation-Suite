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

#include "Utils/DialectUtils.h"
#include "Utils/Error.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

#include "llvm/IR/Module.h"

#include <cmath>
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

/// Pluck out the values from \p controls and \p target whose type is
/// `WireType` and replace all the \p op uses with those values.
void selectWiresAndReplaceUses(Operation *op, ValueRange controls,
                               mlir::Value target) {
  SmallVector<mlir::Value, 4> newWireValues;
  for (const auto &v : controls)
    if (isa<cudaq::quake::WireType>(v.getType()))
      newWireValues.push_back(v);
  if (isa<cudaq::quake::WireType>(target.getType()))
    newWireValues.push_back(target);
  assert(op->getResults().size() == newWireValues.size() &&
         "incorrect number of output wires provided");
  op->replaceAllUsesWith(newWireValues);
}

void selectWiresAndReplaceUses(Operation *op, ValueRange newValues) {
  SmallVector<mlir::Value, 4> newWireValues;
  for (const auto &v : newValues)
    if (isa<cudaq::quake::WireType>(v.getType()))
      newWireValues.push_back(v);
  assert(op->getResults().size() == newWireValues.size() &&
         "incorrect number of output wires provided");
  op->replaceAllUsesWith(newWireValues);
}

void rewriteHToRzXRz(IRRewriter &rewriter, quake::HOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);

  auto c1 = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{c1}, ValueRange{},
                               target);
  rewriter.create<quake::XOp>(loc, false, ValueRange{}, ValueRange{}, target);
  auto c2 = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{c2}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteHToU3(IRRewriter &rewriter, quake::HOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto halfPi =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  auto pi = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  rewriter.create<quake::U3Op>(loc, ValueRange{halfPi, zero, pi}, ValueRange{},
                               ValueRange{target});
  rewriter.eraseOp(op);
}

void rewriteXToHZH(IRRewriter &rewriter, quake::XOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, false, target);
  rewriter.create<quake::ZOp>(loc, false, target);
  rewriter.create<quake::HOp>(loc, false, target);
  rewriter.eraseOp(op);
}

void rewriteXToRx(IRRewriter &rewriter, quake::XOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  rewriter.create<quake::RxOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteYToRy(IRRewriter &rewriter, quake::YOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createQuakeConstOp(loc, rewriter, -M_PI, rewriter.getF64Type());
  rewriter.create<quake::RyOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteZToHXH(IRRewriter &rewriter, quake::ZOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, false, target);
  rewriter.create<quake::XOp>(loc, false, target);
  rewriter.create<quake::HOp>(loc, false, target);
  rewriter.eraseOp(op);
}

void rewriteZToRz(IRRewriter &rewriter, quake::ZOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteSToRz(IRRewriter &rewriter, quake::SOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteSToSdgSdgSdg(IRRewriter &rewriter, quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::SOp>(loc, true, target);
  rewriter.create<quake::SOp>(loc, true, target);
  rewriter.create<quake::SOp>(loc, true, target);
  rewriter.eraseOp(op);
}

void rewriteSToTT(IRRewriter &rewriter, quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::TOp>(loc, false, target);
  rewriter.create<quake::TOp>(loc, false, target);
  rewriter.eraseOp(op);
}

void rewriteSdgToRz(IRRewriter &rewriter, quake::SOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle =
      createQuakeConstOp(loc, rewriter, -M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteSdgToSSS(IRRewriter &rewriter, quake::SOp op) {
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::SOp>(loc, false, target);
  rewriter.create<quake::SOp>(loc, false, target);
  rewriter.create<quake::SOp>(loc, false, target);
  rewriter.eraseOp(op);
}

void rewriteTToRz(IRRewriter &rewriter, quake::TOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle = createQuakeConstOp(loc, rewriter, M_PI_4, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteTdgToRz(IRRewriter &rewriter, quake::TOp op) {
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto angle =
      createQuakeConstOp(loc, rewriter, -M_PI_4, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{angle}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteR1ToRz(IRRewriter &rewriter, quake::R1Op op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{param}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteRxToHRzH(IRRewriter &rewriter, quake::RxOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{param}, ValueRange{},
                               target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteRyToRzRxRz(IRRewriter &rewriter, quake::RyOp op) {
  Location loc = op.getLoc();
  auto rotation = op.getParameters()[0];
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto c1 = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{c1}, ValueRange{},
                               target);
  rewriter.create<quake::RxOp>(loc, false, ValueRange{rotation}, ValueRange{},
                               target);
  auto c2 = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{c2}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteRzToHRxH(IRRewriter &rewriter, quake::RzOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::RxOp>(loc, false, param, ValueRange{}, target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteRzToU3(IRRewriter &rewriter, quake::RzOp op) {
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  rewriter.create<quake::U3Op>(loc, ValueRange{zero, zero, param}, ValueRange{},
                               ValueRange{target});
  rewriter.eraseOp(op);
}

void rewriteU2ToRzRyRz(IRRewriter &rewriter, quake::U2Op op) {
  auto params = op.getParameters();
  auto phi = params[0];
  auto lambda = params[1];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto halfPi =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{phi}, ValueRange{},
                               target);
  rewriter.create<quake::RyOp>(loc, false, ValueRange{halfPi}, ValueRange{},
                               target);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{lambda}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteU2ToU3(IRRewriter &rewriter, quake::U2Op op) {
  auto params = op.getParameters();
  auto phi = params[0];
  auto lambda = params[1];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  auto halfPi =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::U3Op>(loc, ValueRange{halfPi, phi, lambda},
                               ValueRange{}, target);
  rewriter.eraseOp(op);
}

void rewriteU3ToRzRyRz(IRRewriter &rewriter, quake::U3Op op) {
  auto params = op.getParameters();
  auto theta = params[0];
  auto phi = params[1];
  auto lambda = params[2];
  Location loc = op.getLoc();
  ValueRange target = op.getTargets();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{lambda}, ValueRange{},
                               target);
  rewriter.create<quake::RyOp>(loc, false, ValueRange{theta}, ValueRange{},
                               target);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{phi}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteSwapToUpperCxCxCx(IRRewriter &rewriter, quake::SwapOp op) {
  auto q0 = op.getTargets()[0];
  auto q1 = op.getTargets()[1];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::XOp>(loc, q0, q1);
  rewriter.create<quake::XOp>(loc, q1, q0);
  rewriter.create<quake::XOp>(loc, q0, q1);
  rewriter.eraseOp(op);
}

void rewriteCxToUpperHCzH(IRRewriter &rewriter, quake::XOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::ZOp>(loc, control, target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteCzToUpperHCxH(IRRewriter &rewriter, quake::ZOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::XOp>(loc, control, target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteCyToSCxSdg(IRRewriter &rewriter, quake::YOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::SOp>(loc, target);
  rewriter.create<quake::XOp>(loc, control, target);
  rewriter.create<quake::SOp>(loc, true, target);
  rewriter.eraseOp(op);
}

void rewriteCrxToHCrzH(IRRewriter &rewriter, quake::RxOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::RzOp>(loc, false, mlir::Value{param}, control, target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteCrzToHCrxH(IRRewriter &rewriter, quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.create<quake::RxOp>(loc, false, param, control, target);
  rewriter.create<quake::HOp>(loc, target);
  rewriter.eraseOp(op);
}

void rewriteCrzToRzCu3(IRRewriter &rewriter, quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);

  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  mlir::Value halfAngle = createQuakeDivF(loc, angle, 2.0, rewriter);
  mlir::Value negHalfAngle = arith::NegFOp::create(rewriter, loc, halfAngle);

  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{negHalfAngle},
                               ValueRange{}, control);
  rewriter.create<quake::U3Op>(loc, ValueRange{zero, zero, param}, control,
                               target);
  rewriter.eraseOp(op);
}

void rewriteCrzToRzCxRzCx(IRRewriter &rewriter, quake::RzOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  mlir::Value halfAngle = createQuakeDivF(loc, angle, 2.0, rewriter);
  mlir::Value negHalfAngle = arith::NegFOp::create(rewriter, loc, halfAngle);

  rewriter.create<quake::RzOp>(loc, false, ValueRange{halfAngle}, ValueRange{},
                               ValueRange{target});
  rewriter.create<quake::XOp>(loc, ValueRange{control}, ValueRange{target});
  rewriter.create<quake::RzOp>(loc, false, ValueRange{negHalfAngle},
                               ValueRange{}, ValueRange{target});
  rewriter.create<quake::XOp>(loc, ValueRange{control}, ValueRange{target});
  rewriter.eraseOp(op);
}

void rewriteCryToRzCrxRz(IRRewriter &rewriter, quake::RyOp op) {
  auto control = op.getControls()[0];
  auto target = op.getTargets()[0];
  auto param = op.getParameters()[0];
  Location loc = op.getLoc();
  rewriter.setInsertionPointAfter(op);
  auto negHalfPi =
      createQuakeConstOp(loc, rewriter, -M_PI_2, rewriter.getF64Type());
  auto halfPi =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  rewriter.create<quake::RzOp>(loc, false, ValueRange{negHalfPi}, ValueRange{},
                               target);
  rewriter.create<quake::RxOp>(loc, false, param, control, target);
  rewriter.create<quake::RzOp>(loc, false, ValueRange{halfPi}, ValueRange{},
                               target);
  rewriter.eraseOp(op);
}

void rewriteHToPhasedRx(IRRewriter &rewriter, quake::HOp op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  rewriter.setInsertionPointAfter(op);
  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  auto pi = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  auto pi_2 = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());

  std::array<mlir::Value, 2> parameters = {pi_2, pi_2};
  rewriter.create<quake::PhasedRxOp>(loc, parameters, noControls, target);
  parameters[0] = pi;
  parameters[1] = zero;
  rewriter.create<quake::PhasedRxOp>(loc, parameters, noControls, target);

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
}

void rewriteXToPhasedRx(IRRewriter &rewriter, cudaq::quake::XOp op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  rewriter.setInsertionPointAfter(op);
  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  auto pi = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());

  SmallVector<mlir::Value> parameters = {pi, zero};
  rewriter.create<quake::PhasedRxOp>(loc, parameters, noControls, target);

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
}

void rewriteRxToPhasedRx(IRRewriter &rewriter, cudaq::quake::RxOp op) {
  if (!op.getControls().empty())
    return;

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
  mlir::Value zero =
      createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());

  SmallVector<mlir::Value> parameters = {angle, zero};
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
}

void rewriteRzToPhasedRx(IRRewriter &rewriter, cudaq::quake::RzOp op) {
  if (!op.getControls().empty())
    return;

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
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  auto pi_2 = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  auto negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);
  auto negAngle = arith::NegFOp::create(rewriter, loc, angle);

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negAngle;
  parameters[1] = pi_2;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negPi_2;
  parameters[1] = zero;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);

  // selectWiresAndReplaceUses(op, target);
  rewriter.eraseOp(op);
}

void rewriteR1ToPhasedRx(IRRewriter &rewriter, cudaq::quake::R1Op op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle = op.getParameter();
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);
  auto angleType = op.getParameter().getType();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto zero = createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  auto pi_2 = createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  auto negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);
  auto negAngle = arith::NegFOp::create(rewriter, loc, angle);

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negAngle;
  parameters[1] = pi_2;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negPi_2;
  parameters[1] = zero;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);

  rewriter.eraseOp(op);
}

void rewriteYToPhasedRx(IRRewriter &rewriter, cudaq::quake::YOp op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  auto pi = createQuakeConstOp(loc, rewriter, M_PI, rewriter.getF64Type());
  auto negPi_2 =
      createQuakeConstOp(loc, rewriter, -M_PI_2, rewriter.getF64Type());

  SmallVector<mlir::Value> parameters = {pi, negPi_2};

  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);

  rewriter.eraseOp(op);
}

void rewriteSToPhasedRx(IRRewriter &rewriter, cudaq::quake::SOp op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  mlir::Value zero =
      createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  mlir::Value pi_2 =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  mlir::Value negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);

  mlir::Value angle = op.isAdj() ? pi_2 : negPi_2;

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = angle;
  parameters[1] = pi_2;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negPi_2;
  parameters[1] = zero;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  rewriter.eraseOp(op);
}

void rewriteTToPhasedRx(IRRewriter &rewriter, cudaq::quake::TOp op) {
  if (!op.getControls().empty())
    return;

  // Op info
  Location loc = op->getLoc();
  mlir::Value target = op.getTarget();
  mlir::Value angle =
      createQuakeConstOp(loc, rewriter, -M_PI_4, rewriter.getF64Type());
  if (op.isAdj())
    angle = arith::NegFOp::create(rewriter, loc, angle);

  // Necessary/Helpful constants
  SmallVector<mlir::Value> noControls;
  mlir::Value zero =
      createQuakeConstOp(loc, rewriter, 0.0, rewriter.getF64Type());
  mlir::Value pi_2 =
      createQuakeConstOp(loc, rewriter, M_PI_2, rewriter.getF64Type());
  mlir::Value negPi_2 = arith::NegFOp::create(rewriter, loc, pi_2);

  std::array<mlir::Value, 2> parameters = {pi_2, zero};

  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = angle;
  parameters[1] = pi_2;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);
  parameters[0] = negPi_2;
  parameters[1] = zero;
  rewriter.create<cudaq::quake::PhasedRxOp>(loc, parameters, noControls,
                                            target);

  rewriter.eraseOp(op);
}

//===----------------------------------------------------------------------===//
// The rule table: for every recognized "native gate mnemonic", the list of
// known ways to build it out of other gates. Multiple entries mean multiple
// choices exist in lib/Passes/Decompositions/ for that gate.
//===----------------------------------------------------------------------===//

struct DecompositionRule {
  const char *name;
  std::vector<std::string> produces;
  std::function<void(IRRewriter &, Operation *)> apply;
};

template <typename QuakeOpT>
DecompositionRule makeRule(const char *name, std::vector<std::string> produces,
                           void (*rewrite)(IRRewriter &, QuakeOpT)) {
  return DecompositionRule{name, std::move(produces),
                           [rewrite](IRRewriter &rewriter, Operation *op) {
                             rewrite(rewriter, cast<QuakeOpT>(op));
                           }};
}
