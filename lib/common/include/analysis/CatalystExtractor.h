/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

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
*************************************************************************
  author Akshay Bhosale
  date   February 2026
  version 1.0
*************************************************************************/

#include "Extractor.h"
#include "Utils/quantumutils.h"
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

    SmallVector<func::FuncOp, 16> QuantumKernels;
    auto walkResult = module.walk([&](Operation *op) {
      // Check if it is a quantum kernel
      // TODO (Akshay): Check here for catalyst kernel?
      if (auto funcOp = dyn_cast<func::FuncOp>(op)) {
        // TODO: Is this a solid check?
        funcOp.walk([&](Operation *fop) {
          if (fop->getDialect()->getNamespace() == "quantum") {
            QuantumKernels.push_back(funcOp);
            return WalkResult::interrupt();
          }
          return WalkResult::advance();
        });

        // Skip functions which are not quantum kernels
        return WalkResult::skip();
      }
      return WalkResult::advance();
    });
    return QuantumKernels;
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
            assert(!kernelInfo.AllocOp &&
                   "Can only have 1 alloc instruction in a quantum kernel!");
            kernelInfo.AllocOp = allocaOp;
            return;
          }

          auto view = createQuantumView(Op, kernelInfo.NumMeasureQubits);
          kernelInfo.OpQViewMap[Op] = std::move(view);
        }
      });
      KernelDialectInfo[kernel] = std::move(kernelInfo);
    }
  }

  void addOperation(Operation *NewOp) override {
    auto funcOp = NewOp->getParentOfType<mlir::func::FuncOp>();
    assert(funcOp && "Adding New O: Parent Function not found!");
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    assert(!QInfo.count(NewOp) &&
           "Adding New Op: Op already present in QunatumInfoMap");
    auto view =
        createQuantumView(NewOp, KernelDialectInfo[funcOp].NumMeasureQubits);
    QInfo[NewOp] = view;
  }

  bool UpdateOperands(Operation *Op, QubitRole Role, Value OrigValue,
                      Value NewValue) {
    auto funcOp = getOpParentFunc(Op);
    assert(KernelDialectInfo.count(funcOp) && "No QuantumOpInfo for funcOp");
    auto &QInfo = KernelDialectInfo[funcOp].OpQViewMap;
    assert(QInfo.count(Op) && "Updating Op: Op not present in QunatumInfoMap");
    QuantumOpView &OpQView = QInfo[Op];
    auto &Operands = OpQView.getQubits(Role).in;
    auto it = std::find(Operands.begin(), Operands.end(), OrigValue);
    if (it == Operands.end()) {
      return false; // originalValue not found
    }
    *it = NewValue;
    return true;
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

  mlir::LogicalResult verifyModule() override { return module.verify(); }

private:
  ModuleOp module;
  MapVector<func::FuncOp, QuantumKernelInfo> KernelDialectInfo;
  int nextClassicalBit = 0;

  QuantumOpView createQuantumView(Operation *Op, size_t &NumMeasureQubits) {
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

      assert(!OpRoles.empty() &&
             "Found a gate Op with empty Operand Roles(Control/Target)");
      assert((OpRoles.size() == g.getQubitOperands().size()) &&
             "Operand Roles not equals No. of Qubit Operands");

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
    assert(funcOp && "Adding New O: Parent Function not found!");
    return funcOp;
  }

  int64_t getMeasuredQubitCount(catalyst::quantum::MeasureOp op,
                                SmallVector<MeasurementInfo> &measurements) {
    int64_t count = 0;

    for (auto operand : op->getOperands()) {
      if (mlir::isa<catalyst::quantum::QubitType>(operand.getType())) {
        auto originOpQubit = getOriginQubit(operand);
        measurements.push_back({originOpQubit->index, nextClassicalBit++});
        ++count;
      }
    }

    return count;
  }
};
