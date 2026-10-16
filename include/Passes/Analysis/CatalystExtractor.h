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

#include "Extractor.h"
#include "Utils/DialectUtils.h"

#include <llvm/Support/raw_ostream.h>

// static const StringSet<> rotationsSet = {"RX", "RY", "RZ"};
// static const StringSet<> hermitianSet = {"Hadamard", "PauliX", "PauliY",
// "PauliZ",
//                                          "H",        "X",      "Y", "Z"};
// static const StringSet<> multiQubitSet = {"CNOT", "CZ", "SWAP"};

class CatalystQuantumAnalysis : public MyModuleAnalysis {

public:
  CatalystQuantumAnalysis(ModuleOp module) : module(module) { gatherOpInfo(); }

  SmallVector<func::FuncOp, 16> fetchQuantumKernels() override {
    SmallVector<func::FuncOp, 16> kernels;
    for (auto funcOp : module.getOps<func::FuncOp>()) {
      if (funcOp.isExternal())
        continue;
      bool hasQuantumOp =
          funcOp
              .walk([](Operation *op) {
                return op->getDialect()->getNamespace() == "quantum"
                           ? WalkResult::interrupt()
                           : WalkResult::advance();
              })
              .wasInterrupted();
      if (hasQuantumOp)
        kernels.push_back(funcOp);
    }
    return kernels;
  }

  void gatherOpInfo() override {
    auto QuantumKernels = fetchQuantumKernels();

    for (auto kernel : QuantumKernels) {
      QuantumKernelInfo kernelInfo;

      kernel.getBody().walk([&](Operation *Op) {
        if (Op->getDialect()->getNamespace() == "quantum") {
          // if (auto measop = dyn_cast<catalyst::quantum::MeasureOp>(Op)) {
          //     kernelInfo.NumMeasureQubits += getMeasuredQubitCount(measop);
          //     return;
          // }
          if (auto allocaOp = dyn_cast<catalyst::quantum::AllocOp>(Op)) {
            kernelInfo.AllocatedQubits = allocaOp.getNqubitsAttr().value();
            if (kernelInfo.AllocOp) {
              mlir::emitError(allocaOp->getLoc(),
                              "CatalystExtractor: gaterOpInfo: Can Only have 1 "
                              "allocaOp in a kernel");
              _valid = false;
              return;
            }
            kernelInfo.AllocOp = allocaOp;
            return;
          }

          auto view = createQuantumView(Op, kernelInfo.NumMeasureQubits);
          if (!view) {
            mlir::emitError(kernel->getLoc(),
                            "CatalystExtractor: gaterOpInfo: Could not create "
                            "QuantumView for Kernel");
            _valid = false;
            return;
          }
          kernelInfo.OpQViewMap[Op] = std::move(*view);
        }
      });
      KernelDialectInfo[kernel] = std::move(kernelInfo);
    }
  }

  bool addOperation(Operation *NewOp) override {
    auto funcOp = NewOp->getParentOfType<mlir::func::FuncOp>();
    if (!funcOp) {
      mlir::emitError(
          NewOp->getLoc(),
          "CatalystExtractor: Adding New Op: Parent Function not found!");
      return false;
    }
    if (!KernelDialectInfo.count(funcOp)) {
      mlir::emitError(
          NewOp->getLoc(),
          "CatalystExtractor: Adding New Op: No QuantumOpInfo for funcOp");
      return false;
    }
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    if (QInfo.count(NewOp)) {
      mlir::emitError(NewOp->getLoc(), "CatalystExtractor: Adding New Op: Op "
                                       "already present in QuantumInfoMap");
      return false;
    }
    auto view =
        createQuantumView(NewOp, KernelDialectInfo[funcOp].NumMeasureQubits);
    if (!view) {
      mlir::emitError(NewOp->getLoc(),
                      "CatalystExtractor: addOperation: Could not create "
                      "QuantumView for Op");
      return false;
    }
    QInfo[NewOp] = *view;
    return true;
  }

  bool UpdateOperands(Operation *Op, QubitRole Role, mlir::Value OrigValue,
                      mlir::Value NewValue) {
    auto funcOp = getOpParentFunc(Op);
    if (!KernelDialectInfo.count(funcOp)) {
      mlir::emitError(Op->getLoc(),
                      "CatalystExtractor: No QuantumOpInfo for funcOp");
      return false;
    }
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    if (!QInfo.count(Op)) {
      mlir::emitError(Op->getLoc(),
                      "CatalystExtractor: Op not present in QuantumInfoMap");
      return false;
    }
    QuantumOpView &OpQView = QInfo[Op];
    auto &Operands = OpQView.getQubits(Role).in;
    auto it = std::find(Operands.begin(), Operands.end(), OrigValue);
    if (it == Operands.end()) {
      return false; // originalValue not found
    }
    *it = NewValue;
    return true;
  }

  bool clearKernelBody(func::FuncOp &kernel,
                       std::set<Operation *> notToErase) override {
    // cleaning the mlir::funcOp corresponding to the quake circuit
    if (!KernelDialectInfo.count(kernel)) {
      mlir::emitError(kernel->getLoc(),
                      "CatalystExtractor: No QuantumOpInfo for funcOp");
      return false;
    }
    auto &QInfo = KernelDialectInfo[kernel].OpQViewMap;
    for (auto &[Op, qview] : QInfo) {
      if (notToErase.count(Op))
        continue;
      QInfo.erase(Op);
      Op->erase();
    }
    return true;
  }

  MapVector<func::FuncOp, QuantumKernelInfo> getKernelDialectInfo() override {
    return KernelDialectInfo;
  }

  const std::optional<QuantumOpView> getOpInfo(Operation *Op) override {
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    if (!KernelDialectInfo.count(funcOp)) {
      mlir::emitError(
          Op->getLoc(),
          "CatalystExtractor: Get Op Info: No QuantumOpInfo for funcOp");
      return std::nullopt;
    }
    auto QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    if (!QInfo.count(Op)) {
      mlir::emitError(
          Op->getLoc(),
          "CatalystExtractor: Get Op Info: Op not present QuantumInfoMap");
      return std::nullopt;
    }
    return QInfo[Op];
  }

  mlir::LogicalResult verifyModule() override { return module.verify(); }
  [[nodiscard]] bool isValid() const override { return _valid; }

private:
  ModuleOp module;
  MapVector<func::FuncOp, QuantumKernelInfo> KernelDialectInfo;
  int nextClassicalBit = 0;
  bool _valid = true;

  std::optional<QuantumOpView> createQuantumView(Operation *Op,
                                                 size_t &NumMeasureQubits) {
    QuantumOpView view;
    if (hasQuantumEffect(Op)) {
      view.hasSideEffects = true;
    }
    // Only consider operations on gates with side-effects
    if (auto g = isCatalystQuantumGateOp(Op)) {
      // TODO isAdj initialization?
      auto gateName = g.getGateName();
      view.isAdjoint = g.getAdjointFlag(); // Is this correct?

      if (gateName == "S" && view.isAdjoint)
        gateName = "SAdj";
      else if ((gateName == "Hadamard" || gateName == "H") &&
               (g.getQubitOperands().size() == 2)) {
        gateName = "CH";
      }

      view.GateTy = parseGateTy(gateName);

      std::vector<QubitRole> OpRoles =
          getGateOpRoles(gateName); // Now we can separate out the
                                    // Qubits into Ctrl/Target

      if (OpRoles.empty()) {
        mlir::emitError(g->getLoc(), "CatalystExtractor: Found a gate Op with "
                                     "empty Operand Roles(Control/Target)");
        return std::nullopt;
      }
      if (OpRoles.size() != g.getQubitOperands().size()) {
        mlir::emitError(g->getLoc(), "CatalystExtractor: Operand Roles not "
                                     "equals No. of Qubit Operands");
        return std::nullopt;
      }

      for (auto p : g.getParams()) {
        view.params.push_back(p);
      }

      for (unsigned i = 0; i < g.getQubitOperands().size(); i++) {
        QubitID ID;
        QubitRole role = OpRoles[i];

        auto gop = g.getQubitOperands()[i];
        auto resop = g->getResults()[i];

        if (role == QubitRole::Control) {
          ID.base = gop;
          ID.index = -1;
          view.getQubits(QubitRole::Control).ids.push_back(ID);
          view.getQubits(QubitRole::Control).in.push_back(gop);
          view.getQubits(QubitRole::Control).out.push_back(resop);
        }
        if (role == QubitRole::Target) {
          ID.base = gop;
          ID.index = -1;
          view.getQubits(QubitRole::Target).ids.push_back(ID);
          view.getQubits(QubitRole::Target).in.push_back(gop);
          view.getQubits(QubitRole::Target).out.push_back(resop);
        }
      }
    }
    if (auto measop = dyn_cast<catalyst::quantum::MeasureOp>(Op)) {
      view.isMeasureOp = true;
      NumMeasureQubits += getMeasuredQubitCount(measop, view.measurements);

      for (unsigned i = 0; i < measop->getNumOperands(); i++) {
        QubitID ID;
        auto resop = measop->getResults()[i];

        auto gop = measop->getOperand(i);
        ID.base = gop;
        ID.index = -1;
        view.getQubits(QubitRole::Target).ids.push_back(ID);
        view.getQubits(QubitRole::Target).in.push_back(gop);
        view.getQubits(QubitRole::Target).out.push_back(resop);
      }
    }

    // llvm::outs() << "Op: " << *Op << "\n";
    // for(auto c : OpView.ControlQubits){
    //   llvm::outs().indent(4) << " Ctrl: " << c.base << "\n";
    // }
    //  for(auto t : OpView.TargetQubits){
    //   llvm::outs().indent(4) << " t: " << t.base << "\n";
    // }
    return view;
  }

  mlir::func::FuncOp getOpParentFunc(Operation *Op) {
    auto funcOp = Op->getParentOfType<mlir::func::FuncOp>();
    return funcOp;
  }

  int64_t getMeasuredQubitCount(catalyst::quantum::MeasureOp op,
                                SmallVector<MeasurementInfo> &measurements) {
    int64_t count = 0;

    for (auto operand : op->getOperands()) {
      if (mlir::isa<catalyst::quantum::QubitType>(operand.getType())) {
        auto originOpQubit = getOriginQubit(operand);
        if (!originOpQubit) {
          mlir::emitError(
              operand.getLoc(),
              "CatalystExtractor: Origin Qubit of operand not found");
          continue;
        }
        measurements.push_back({originOpQubit->index, nextClassicalBit++});
        ++count;
      }
    }

    return count;
  }
};
