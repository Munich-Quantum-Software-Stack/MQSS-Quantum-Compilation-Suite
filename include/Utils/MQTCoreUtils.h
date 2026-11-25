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

#include "Utils/TransformUtils.h"
#include "ir/QuantumComputation.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"

#include <llvm/Support/Error.h>
#include <mlir/Dialect/Utils/StaticValueUtils.h>
#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/Location.h>
#include <mlir/IR/OpDefinition.h>
#include <mlir/IR/Value.h>

using namespace mlir;

// Checks `opfr` resolves to a constant qubit index; on success binds `name`
// as a plain int64_t in the current scope, else emits a diagnostic and
// returns `retval` from the enclosing function.
#define GET_QUBIT_INDEX_OR_RETURN(name, opfr, loc, retval)                     \
  auto name##_opt = toQubitIndex(opfr);                                        \
  if (!name##_opt) {                                                           \
    mlir::emitError(loc, "Could not resolve a constant qubit index");          \
    return retval;                                                             \
  }                                                                            \
  int64_t name = static_cast<int64_t>(*name##_opt);

static std::optional<int64_t> toQubitIndex(mlir::OpFoldResult Opfr) {
  std::optional<int64_t> idx = mlir::getConstantIntValue(Opfr);
  return idx;
}

static std::optional<double> getConstantDouble(mlir::Value v) {
  auto *defOp = v.getDefiningOp();
  if (!defOp)
    return std::nullopt;

  if (auto c = dyn_cast<mlir::arith::ConstantOp>(defOp)) {
    if (auto attr = dyn_cast<mlir::FloatAttr>(c.getValue()))
      return attr.getValueAsDouble();
    return std::nullopt;
  }
  if (auto n = dyn_cast<mlir::arith::NegFOp>(defOp)) {
    auto x = getConstantDouble(n.getOperand());
    return x ? std::optional<double>(-*x) : std::nullopt;
  }
  if (auto e = dyn_cast<mlir::arith::ExtFOp>(defOp))
    return getConstantDouble(e.getIn());

  auto binary = [&](auto op, auto fn) -> std::optional<double> {
    auto l = getConstantDouble(op.getLhs());
    auto r = getConstantDouble(op.getRhs());
    if (!l || !r)
      return std::nullopt;
    return fn(*l, *r);
  };
  if (auto a = dyn_cast<mlir::arith::AddFOp>(defOp))
    return binary(a, [](double l, double r) { return l + r; });
  if (auto s = dyn_cast<mlir::arith::SubFOp>(defOp))
    return binary(s, [](double l, double r) { return l - r; });
  if (auto m = dyn_cast<mlir::arith::MulFOp>(defOp))
    return binary(m, [](double l, double r) { return l * r; });
  if (auto d = dyn_cast<mlir::arith::DivFOp>(defOp))
    return binary(d, [](double l, double r) { return l / r; });
  return std::nullopt;
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

  for (auto entry : qview.measurements) {
    auto qcQubit = toQubitIndex(entry.QubitIndex);
    if (!qcQubit.has_value()) {
      mlir::emitError(loc,
                      "Mapping: requires statically-known qubit indices; "
                      "got a dynamic quake.extract_ref/quantum.extract index");
      return false;
    }
    qc.measure(static_cast<qc::Qubit>(*qcQubit), entry.ClassicalBitIndex);
  }
  return true;
}

// Resolves `n` constant angles, or emits an error and returns nullopt.
static std::optional<SmallVector<double, 3>>
getConstantAngles(mlir::Operation *gateOp, ArrayRef<mlir::Value> params,
                  size_t n, StringRef gateName) {
  if (params.size() != n) {
    mlir::emitError(gateOp->getLoc())
        << "Mapping: " << gateName << " gate should have " << n
        << " parameter(s)!";
    return std::nullopt;
  }
  SmallVector<double, 3> angles;
  for (mlir::Value p : params) {
    auto v = getConstantDouble(p);
    if (!v) {
      mlir::emitError(gateOp->getLoc())
          << "Mapping: No constant angle found to create " << gateName
          << " gate!";
      return std::nullopt;
    }
    angles.push_back(*v);
  }
  return angles;
}

static bool loadGates(mlir::Operation *gateOp, qc::QuantumComputation &qc,
                      Gate GateTy, SmallVector<mlir::Value, 2> params,
                      mlir::OpFoldResult controlQubit,
                      mlir::OpFoldResult targetQubit1,
                      mlir::OpFoldResult targetQubit2, bool isAdjoint) {

  // llvm::outs() << "Gate Op: " << *gateOp << " , Ty: " << GateTy << "\n";
  // Adjoint handling (isAdjoint comes from QuantumOpView):
  //  - Self-inverse gates (H, X, Y, Z, CNOT, CY, CZ, CH, SWAP): flag ignored.
  //  - SAdj/TAdj: both extractors fold S/T + adjoint into these types but
  //    leave isAdjoint set, so the flag is ignored for them (no double
  //    inversion). S and T therefore never arrive with isAdjoint set.
  //  - SX: sxdg when adjoint.
  //  - Rotations (RX, RY, RZ, R1, CRX, CRY, CRZ, PhasedRx): negate the angle.
  //  - U2, U3: emitted as the equivalent inverse U3.
  const double sign = isAdjoint ? -1.0 : 1.0;
  // TODO: Load more gates into qc. Add to the following list.
  switch (GateTy) {
  case Gate::H:
  case Gate::Hadamard: {
    GET_QUBIT_INDEX_OR_RETURN(HtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.h(HtgtIdx);
    break;
  }
  case Gate::CNOT: {
    GET_QUBIT_INDEX_OR_RETURN(cxctrlIdx, controlQubit, gateOp->getLoc(), false);
    GET_QUBIT_INDEX_OR_RETURN(cxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.cx(cxctrlIdx, cxtgtIdx);
    break;
  }
  case Gate::CY: {
    GET_QUBIT_INDEX_OR_RETURN(cyctrlIdx, controlQubit, gateOp->getLoc(), false);
    GET_QUBIT_INDEX_OR_RETURN(cytgtIdx, targetQubit1, gateOp->getLoc(), false)
    qc.cy(cyctrlIdx, cytgtIdx);
    break;
  }
  case Gate::CZ: {
    GET_QUBIT_INDEX_OR_RETURN(czctrlIdx, controlQubit, gateOp->getLoc(), false);
    GET_QUBIT_INDEX_OR_RETURN(cztgtIdx, targetQubit1, gateOp->getLoc(), false)
    qc.cz(czctrlIdx, cztgtIdx);
    break;
  }
  case Gate::PauliX: {
    GET_QUBIT_INDEX_OR_RETURN(XtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.x(XtgtIdx);
    break;
  }
  case Gate::PauliY: {
    GET_QUBIT_INDEX_OR_RETURN(YtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.y(YtgtIdx);
    break;
  }
  case Gate::PauliZ: {
    GET_QUBIT_INDEX_OR_RETURN(ZtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.z(ZtgtIdx);
    break;
  }
  case Gate::S: {
    GET_QUBIT_INDEX_OR_RETURN(StgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.s(StgtIdx);
    break;
  }
  case Gate::SAdj: {
    GET_QUBIT_INDEX_OR_RETURN(SdjtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.sdg(SdjtgtIdx);
    break;
  }
  case Gate::T: {
    GET_QUBIT_INDEX_OR_RETURN(TtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.t(TtgtIdx);
    break;
  }
  case Gate::TAdj: {
    GET_QUBIT_INDEX_OR_RETURN(TdjtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.tdg(TdjtgtIdx);
    break;
  }
  case Gate::RX: {
    auto angles = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!angles)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(RxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.rx(sign * angles.value().back(), RxtgtIdx);
    break;
  }
  case Gate::RY: {
    auto angles = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!angles)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(RytgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.ry(sign * angles.value().back(), RytgtIdx);
    break;
  }
  case Gate::RZ: {
    auto angles = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!angles)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(RztgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.rz(sign * angles.value().back(), RztgtIdx);
    break;
  }
  case Gate::SWAP: {
    GET_QUBIT_INDEX_OR_RETURN(Swptgt1Idx, targetQubit1, gateOp->getLoc(),
                              false);
    GET_QUBIT_INDEX_OR_RETURN(Swptgt2Idx, targetQubit2, gateOp->getLoc(),
                              false);
    qc.swap(Swptgt1Idx, Swptgt2Idx);
    break;
  }
  case Gate::PhasedRx: {
    auto angles = getConstantAngles(gateOp, params, 2, parseGateTy(GateTy));
    if (!angles)
      return false;

    GET_QUBIT_INDEX_OR_RETURN(PRxtgt2Idx, targetQubit1, gateOp->getLoc(),
                              false);
    // R(theta, phi)^dagger == R(-theta, phi)
    qc.r(sign * angles.value()[0], angles.value()[1], PRxtgt2Idx);
    break;
  }
  case Gate::SX: {
    GET_QUBIT_INDEX_OR_RETURN(SxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    if (isAdjoint)
      qc.sxdg(SxtgtIdx);
    else
      qc.sx(SxtgtIdx);
    break;
  }
  case Gate::R1: { // R1(theta) == MQT's P(theta)
    auto a = getConstantAngles(gateOp, params, 1, "R1");
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(R1tgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.p(sign * (*a)[0], R1tgtIdx);
    break;
  }
  case Gate::CH: {
    GET_QUBIT_INDEX_OR_RETURN(chctrlIdx, controlQubit, gateOp->getLoc(), false);
    GET_QUBIT_INDEX_OR_RETURN(chtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.ch(chctrlIdx, chtgtIdx);
    break;
  }
  case Gate::CRX: {
    auto a = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(crxctrlIdx, controlQubit, gateOp->getLoc(),
                              false);
    GET_QUBIT_INDEX_OR_RETURN(crxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.crx(sign * (*a)[0], crxctrlIdx, crxtgtIdx);
    break;
  }
  case Gate::CRY: {
    auto a = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(crxctrlIdx, controlQubit, gateOp->getLoc(),
                              false);
    GET_QUBIT_INDEX_OR_RETURN(crxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.cry(sign * (*a)[0], crxctrlIdx, crxtgtIdx);
    break;
  }
  case Gate::CRZ: {
    auto a = getConstantAngles(gateOp, params, 1, parseGateTy(GateTy));
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(crxctrlIdx, controlQubit, gateOp->getLoc(),
                              false);
    GET_QUBIT_INDEX_OR_RETURN(crxtgtIdx, targetQubit1, gateOp->getLoc(), false);
    qc.crz(sign * (*a)[0], crxctrlIdx, crxtgtIdx);
    break;
  }
  case Gate::U2: {
    auto a = getConstantAngles(gateOp, params, 2, parseGateTy(GateTy));
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(U2tgtIdx, targetQubit1, gateOp->getLoc(), false);
    if (isAdjoint) {
      // U2(phi, lambda) == U3(pi/2, phi, lambda), so the adjoint is
      // U3(-pi/2, -lambda, -phi).
      qc.u(-qc::PI / 2, -(*a)[1], -(*a)[0], U2tgtIdx);
    } else {
      qc.u2((*a)[0], (*a)[1], U2tgtIdx); // (phi, lambda)
    }
    break;
  }
  case Gate::U3: {
    auto a = getConstantAngles(gateOp, params, 3, parseGateTy(GateTy));
    if (!a)
      return false;
    GET_QUBIT_INDEX_OR_RETURN(U3tgtIdx, targetQubit1, gateOp->getLoc(), false);
    if (isAdjoint) {
      // U3(theta, phi, lambda)^dagger == U3(-theta, -lambda, -phi)
      qc.u(-(*a)[0], -(*a)[2], -(*a)[1], U3tgtIdx);
    } else {
      qc.u((*a)[0], (*a)[1], (*a)[2], U3tgtIdx); // (theta, phi, lambda)
    }
    break;
  }
  default:
    mlir::emitError(gateOp->getLoc())
        << "Mapping: gate '" << parseGateTy(GateTy)
        << "' is not supported by the OpenQASM backend";
    return false;
  }
  return true;
}

static bool resolveIdx(const QubitID &q, mlir::OpFoldResult &out) {

  if (q.index) {
    out = q.index;
    return true;
  }
  auto origin = getOriginQubit(q.base);
  if (!origin) {
    mlir::emitError(q.base.getLoc(),
                    "Mapping: Could not fetch origin Qubit of operand!");
    return false;
  }
  out = origin->index;

  return true;
}

static bool loadGateOpsIntoQC(mlir::Operation *gateOp,
                              const QuantumOpView &qview,
                              qc::QuantumComputation &qc,
                              bool isControlled = false) {

  mlir::OpFoldResult controlQubitIdx = nullptr;
  mlir::OpFoldResult targetQubit1Idx = nullptr;
  mlir::OpFoldResult targetQubit2Idx = nullptr;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;

  if (isControlled) {
    auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
    if ((controlQubitVector.size() != 1)) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: Gates with 2 Control Qubits not supported");
      return false;
    }

    if (!resolveIdx(controlQubitVector[0], controlQubitIdx)) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: Could not resolve the Index of Control Qubit!");
      return false;
    }
  }
  if (targetQubitVector.size() != 1 && targetQubitVector.size() != 2) {
    mlir::emitError(gateOp->getLoc(),
                    "Mapping: Gates with 1 or 2 target Qubits supported!");
    return false;
  }

  if (targetQubitVector.size() == 2) {
    targetQubit2Idx = targetQubitVector[1].index;
  }

  if (!resolveIdx(targetQubitVector[0], targetQubit1Idx)) {
    mlir::emitError(gateOp->getLoc(),
                    "Mapping: Could not resolve the Index of Target Qubit!");
    return false;
  }

  if (qview.GateTy == Gate::SWAP) {
    if (targetQubitVector.size() != 2) {
      mlir::emitError(gateOp->getLoc(),
                      "Mapping: SWAP gate requires exactly 2 target Qubits!");
      return false;
    }
    if (!targetQubit2Idx) {
      // Gate operation in Catalyst (value semantics)
      // Qubit in Catalyst
      if (!resolveIdx(targetQubitVector[1], targetQubit2Idx)) {
        mlir::emitError(
            gateOp->getLoc(),
            "Mapping: Could not resolve the Index of Target Qubit!");
        return false;
      }
    }
  }
  return loadGates(gateOp, qc, qview.GateTy, qview.params, controlQubitIdx,
                   targetQubit1Idx, targetQubit2Idx, qview.isAdjoint);
}

static llvm::Expected<qc::QuantumComputation>
createMQTQuantumComputation(mlir::Location loc,
                            const QuantumKernelInfo &kernel_info) {
  qc::QuantumComputation qc{kernel_info.AllocatedQubits,
                            kernel_info.NumMeasureQubits};

  for (auto &[Op, OpQView] : kernel_info.OpQViewMap) {
    if (OpQView.GateTy != Gate::UNKNOWN) {
      if (!loadGateOpsIntoQC(Op, OpQView, qc, OpQView.isControlled())) {
        return llvm::createStringError("Mapping: Error when loading GateOp "
                                       "into MQT's QuantumComputation");
      }
    } else if (!OpQView.isMeasureOp && !isIgnorableStructuralOp(Op)) {
      mlir::emitError(Op->getLoc()) << "Mapping: operation '" << Op->getName()
                                    << "' has no OpenQASM equivalent";
      return llvm::createStringError("Mapping: unsupported operation");
    }

    if (OpQView.isMeasureOp) {
      if (!loadMeasureOpIntoQC(OpQView, qc, loc)) {
        return llvm::createStringError("Mapping: Error when loading MeasureOp "
                                       "into MQT's QuantumComputation");
      }
    }
  }

  return qc;
}
