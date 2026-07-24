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
*************************************************************************
  author Akshay Bhosale
  co-author: Claude AI Sonnet/Opus
  date   August 2026
  version 2.0.0
*************************************************************************/

#include "Extractor.h"
#include "Utils/dialectutils.h"

#include <llvm/Support/raw_ostream.h>

class QuakeAnalysis : public MyModuleAnalysis {

public:
  QuakeAnalysis(ModuleOp module) : module(module) { gatherOpInfo(); }

  SmallVector<func::FuncOp, 16> fetchQuantumKernels() override {
    SmallVector<func::FuncOp, 16> kernels;
    for (auto funcOp : module.getOps<func::FuncOp>()) {
      if (funcOp.isExternal()) // skip declarations with no body
        continue;
      if (funcOp->hasAttr(cudaq::entryPointAttrName) ||
          llvm::any_of(funcOp.getArguments(), [](BlockArgument arg) {
            return isa<quake::RefType, quake::VeqType>(arg.getType());
          }))
        kernels.push_back(funcOp);
    }
    return kernels;
  }

  // Gather Information (QuantumView) about every operation
  void gatherOpInfo() override {

    auto QuantumKernels = fetchQuantumKernels();

    for (auto kernel : QuantumKernels) {
      QuantumKernelInfo kernelInfo;
      kernel.getBody().walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quake") {

          if (auto allocaOp = dyn_cast<quake::AllocaOp>(Op)) {
            kernelInfo.AllocatedQubits += getAllocatedQubits(allocaOp);
            assert(!kernelInfo.AllocOp &&
                   "Can only have 1 alloc instruction in a quantum kernel!");
            kernelInfo.AllocOp = allocaOp;
            return;
          }
          auto view = createQuantumView(Op, kernelInfo.NumMeasureQubits);
          kernelInfo.OpQViewMap[Op] = view;
        }
      });
      KernelDialectInfo[kernel] = std::move(kernelInfo);
    }
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }

  void addOperation(Operation *NewOp) override {
    auto funcOp = NewOp->getParentOfType<mlir::func::FuncOp>();
    assert(funcOp && "Adding New Op: Parent Function not found!");
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    assert(!QInfo.count(NewOp) &&
           "Adding New Op: Op already present in QunatumInfoMap");
    auto view =
        createQuantumView(NewOp, KernelDialectInfo[funcOp].NumMeasureQubits);
    QInfo[NewOp] = view;
  }

  void clearKernelBody(func::FuncOp &kernel,
                       std::set<Operation *> notToErase) override {
    // cleaning the mlir::funcOp corresponding to the quake circuit
    assert(KernelDialectInfo.count(kernel) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[kernel].OpQViewMap;
    for (auto &[Op, qview] : QInfo) {
      if (notToErase.count(Op))
        continue;
      QInfo.erase(Op);
      Op->erase();
    }
  }

  bool UpdateOperands(Operation *Op, QubitRole Role, mlir::Value OrigValue,
                      mlir::Value NewValue) {
    auto funcOp = getOpParentFunc(Op);
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    assert(QInfo.count(Op) && "Updating Op: Op not present in QunatumInfoMap");
    QuantumOpView OpQView = QInfo[Op];
    auto &Operands = OpQView.getQubits(Role).in;
    auto it = std::find(Operands.begin(), Operands.end(), OrigValue);
    if (it == Operands.end()) {
      return false; // originalValue not found
    }
    *it = NewValue;
    return true;
  }

  MapVector<func::FuncOp, QuantumKernelInfo> getKernelDialectInfo() override {
    return KernelDialectInfo;
  }

  const QuantumOpView getOpInfo(Operation *Op) override {
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    assert(KernelDialectInfo.count(funcOp) &&
           "Get Op Info: No QuantumOpInfo for funcOp");
    auto QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    assert(QInfo.count(Op) && "Get Op Info: Op not present QuantumInfoMap");
    return QInfo[Op];
  }

private:
  ModuleOp module;
  MapVector<func::FuncOp, QuantumKernelInfo> KernelDialectInfo;
  int nextClassicalBit = 0;

  std::tuple<QubitID, mlir::Value> extractQubits(Operation *Operand) {
    QubitID ID;
    if (auto ext_ref = dyn_cast<quake::ExtractRefOp>(Operand)) {
      auto base = ext_ref.getVeq();
      auto index = ext_ref.getConstantIndex();
      ID.base = base;
      ID.index = index;
      return {ID, ext_ref};
    }
    return {ID, nullptr};
  }

  const QuantumOpView createQuantumView(Operation *Op,
                                        size_t &NumMeasureQubits) {

    QuantumOpView view;
    if (!mlir::isMemoryEffectFree(Op)) {
      view.hasSideEffects = true;
    }

    if (auto gate = dyn_cast<quake::OperatorInterface>(Op)) {
      // Gather the Control Qubits
      view.isAdjoint = gate.isAdj();

      for (auto ctrl : gate.getControls()) {
        auto [ID, ext_ref] = extractQubits(ctrl.getDefiningOp());
        view.getQubits(QubitRole::Control).ids.push_back(ID);
        view.getQubits(QubitRole::Control).in.push_back(ext_ref);
      }
      // Gather the Target Qubits
      for (auto t : gate.getTargets()) {
        auto [ID, ext_ref] = extractQubits(t.getDefiningOp());
        view.getQubits(QubitRole::Target).ids.push_back(ID);
        view.getQubits(QubitRole::Target).in.push_back(ext_ref);
      }
      // Gather Parameters if any (E.g. Rotation Angle)
      for (auto p : gate.getParameters()) {
        view.params.push_back(p);
      }
      auto [isQGateOp, GateTy] = isQuakeQuantumGate(Op);
      if (isQGateOp) {
        view.GateTy = parseGateTy(GateTy);
      }
    } else if (auto extract_refop = dyn_cast<quake::ExtractRefOp>(Op)) {
      // Do something
      QubitID ID;
      ID.base = extract_refop.getVeq();
      ID.index = extract_refop.getConstantIndex();
      view.getQubits(QubitRole::Target).ids.push_back(ID);
    } else if (auto quakemeasop = dyn_cast<quake::MeasurementInterface>(Op)) {
      NumMeasureQubits +=
          getMeasurementResultCount(quakemeasop, view.measurements);
      view.isMeasureOp = true;

      for (unsigned i = 0; i < quakemeasop->getNumOperands(); i++) {
        QubitID ID;
        auto resop = quakemeasop->getResults()[i];

        auto gop = quakemeasop->getOperand(i);

        ID.base = gop;
        ID.index = -1;
        view.getQubits(QubitRole::Target).ids.push_back(ID);
        view.getQubits(QubitRole::Target).in.push_back(gop);
        view.getQubits(QubitRole::Target).out.push_back(resop);
      }
    }
    return view;
  }

  mlir::func::FuncOp getOpParentFunc(Operation *Op) {
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    assert(funcOp && "Adding New O: Parent Function not found!");
    return funcOp;
  }

  int64_t
  getMeasurementResultCount(quake::MeasurementInterface meas,
                            SmallVector<MeasurementInfo> &measurements) {
    // Usually the measured qubit/register is operand 0.
    if (meas->getNumOperands() == 0)
      return 0;

    auto operand = meas->getOperand(0);

    if (auto RefOp = dyn_cast<quake::ExtractRefOp>(operand.getDefiningOp())) {
      auto q = static_cast<int64_t>(RefOp.getConstantIndex());
      measurements.push_back({q, nextClassicalBit++});
      return 1;
    }

    mlir::Type measuredTy = operand.getType();
    if (auto veqTy = dyn_cast<quake::VeqType>(measuredTy)) {
      if (veqTy.hasSpecifiedSize()) {
        for (auto q = 0; q < veqTy.getSize(); ++q) {
          measurements.push_back({q, nextClassicalBit++});
        }
        return static_cast<int64_t>(veqTy.getSize());
      }

      return -1; // dynamic-size register
    }

    return -1;
  }

  int64_t getAllocatedQubits(quake::AllocaOp alloc) {
    mlir::Type ty = alloc.getResult().getType();
    if (auto veqTy = mlir::dyn_cast<quake::VeqType>(ty))
      if (veqTy.hasSpecifiedSize()) {
        int n = veqTy.getSize();
        return n;
      }
    return 0;
  }
};
