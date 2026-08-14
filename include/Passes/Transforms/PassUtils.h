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

#include "PassIncludes.h"
#include "Utils/debug_utils.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include <cmath>
#include <llvm/Support/Debug.h>

static void Commute(std::vector<CommuteTy> CommmutationCandidates) {

  for (auto CandidatesInfo : CommmutationCandidates) {
    MQSS_DEBUG("Commuting...\n");
    auto *Cand1 = CandidatesInfo.Op1;
    auto *Cand2 = CandidatesInfo.Op2;
    MQSS_DEBUG("Cand 1: " << *Cand1 << "\n");
    MQSS_DEBUG("Cand 2: " << *Cand2 << "\n");
    Cand1->moveAfter(Cand2);

    MQSS_DEBUG("---------------------------------------------\n");
  }
}

static void Commute(std::vector<CommuteTy> CommmutationCandidates,
                    Comparety CompareKey, MyModuleAnalysis &analysis) {

  auto KeyGate1 = CompareKey.KeyGate1;
  auto KeyGate2 = CompareKey.KeyGate2;

  for (auto OpInfo : CommmutationCandidates) {

    MQSS_DEBUG("SSA Commuting...\n");

    auto *Op1 = OpInfo.Op1;
    auto *Op2 = OpInfo.Op2;
    auto Op1QView = OpInfo.Op1QView;
    auto Op2QView = OpInfo.Op2QView;

    MQSS_DEBUG("Op1: " << *Op1 << "\n");
    MQSS_DEBUG("Op2: " << *Op2 << "\n");

    // Get the Op1 and Op2 Qubits being compared
    // E.g. commuting CNOT (Op1) and Rx (Op2)
    Op1->moveAfter(Op2);
    // Now, CNOT is after Rx
    // auto Op1Operand = OpInfo.Op1QView.getQubits(KeyGate1).in[0];

    // Replace operand (Ctrl/Target) of CNOT with the result of Rx (Ctrl/Target)
    // The CompareKeys are used to determine which operand (Ctrl/Target) is
    // replaced

    assert(!Op1QView.getQubits(KeyGate1).in.empty() &&
           "Op1 does not have operand targets");
    assert(!Op2QView.getQubits(KeyGate2).out.empty() &&
           "Op2 does not have result targets");

    auto Op1Operand = Op1QView.getQubits(KeyGate1).in[0];
    auto Op2Operand = Op2QView.getQubits(KeyGate2).in[0];

    auto Op2Result = Op2QView.getQubits(KeyGate2).out[0];

    // Do something with the Operand of Rx (Op2)
    Op2->replaceUsesOfWith(Op2Operand, Op1Operand);
    Op1->replaceUsesOfWith(Op1Operand, Op2Result);

    MQSS_DEBUG("Op2: " << *Op2 << "\n");
    MQSS_DEBUG("Op1: " << *Op1 << "\n");

    // Update the Op1 and Op2 operands in QuantumOpView
    analysis.UpdateOperands(Op2, KeyGate2, Op2Operand, Op1Operand);
    analysis.UpdateOperands(Op1, KeyGate1, Op1Operand, Op2Result);

    MQSS_DEBUG("---------------------------------------------\n");
  }
}

static std::vector<CommuteTy> performCommutation(MyModuleAnalysis &analysis,
                                                 PassInfoty PassInfo) {

  auto Gate1Ty = PassInfo.FirstGateTy;
  auto Gate2Ty = PassInfo.SecondGateTy;
  auto CompareKey = PassInfo.CompareKey;

  CommuteInfoTy CommutationInfo;
  CommuteInfoTy SSACommuteCandidates;
  for (auto &[kernel, KernelInfo] : analysis.getKernelDialectInfo()) {
    if (KernelInfo.AllocatedQubits == 0)
      continue;
    auto QView = KernelInfo.OpQViewMap;
    for (auto &[Op1, Op1QView] : QView) {
      auto FirstGateTy = Op1QView.GateTy;

      if (!Op1QView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
        continue;

      if (!is_contained(Gate1Ty, FirstGateTy))
        continue;

      if (CommutationInfo.isScheduled(Op1))
        continue;

      auto *Op2 = Op1->getNextNode();
      if (!Op2)
        continue;

      auto Op1CtrlQubits = Op1QView.getQubits(QubitRole::Control);
      auto Op1TargetQubits = Op1QView.getQubits(QubitRole::Target);

      while (Op2) {
        auto nextOpView = QView[Op2];

        auto Op2CtrlQubits = nextOpView.getQubits(QubitRole::Control);
        auto Op2TargetQubits = nextOpView.getQubits(QubitRole::Target);
        //  "next op: " << *NextOp << "\n";
        if (nextOpView.hasSideEffects &&
            !is_contained(Gate2Ty, nextOpView.GateTy)) {
          if (touchesAny(Op2, Op1CtrlQubits.ids, QView) ||
              touchesAny(Op2, Op1TargetQubits.ids, QView)) {

            // "Found intervening ops!\n";
            break;
          }
          if (touchesAny(Op2, Op1TargetQubits.out)) {
            break;
          }
        }

        if (is_contained(Gate2Ty, nextOpView.GateTy)) {

          if (SameQubits({Op1CtrlQubits.ids, Op1TargetQubits.ids},
                         {Op2CtrlQubits.ids, Op2TargetQubits.ids},
                         CompareKey)) {
            CommutationInfo.gather(Op1, Op2);
          } else if (SameQubitValues({Op1CtrlQubits.out, Op1TargetQubits.out},
                                     {Op2CtrlQubits.in, Op2TargetQubits.in},
                                     CompareKey)) {
            // Example:
            // Original:
            //  %out_qubits_2:2 = quantum.custom "CNOT"() %2, %3 : !quantum.bit,
            //  !quantum.bit %out_ctrl = quantum.custom "RX"(%cst)
            //  %out_qubits_2#0 : !quantum.bit
            // Commuted:
            //  %new_ctrl = quantum.custom "RX"(%cst) %2 : !quantum.bit
            //  %new_pair:2 = quantum.custom "CNOT"() %new_ctrl, %3 :
            //  !quantum.bit, !quantum.bit
            SSACommuteCandidates.gather(Op1, Op2, Op1QView, nextOpView);
          }
          // TODO: For catalyst - Cannot commute by checking the output qubits
          // and input qubits of Gate Ops. If the check returns true, and the
          // gates are commuted, it will break SSA form (Def-Use will be
          // commuted to Use-Def). "Not same
          // Qubits!\n";
          break;
        }

        Op2 = Op2->getNextNode();
      }
    }
  }

  auto CommuteCandidates = CommutationInfo.getCommutationCandidates();
  if (!CommuteCandidates.empty()) {
    Commute(CommuteCandidates);
  } else {
    CommuteCandidates = SSACommuteCandidates.getCommutationCandidates();
    if (!CommuteCandidates.empty()) {
      Commute(CommuteCandidates, CompareKey, analysis);
    } else {
      MQSS_DEBUG("-----No Commutation candidates for the kernel----\n");
    }
  }

  return CommuteCandidates;
}

static void performCommuteAndSwitch(MyModuleAnalysis &analysis,
                                    PassInfoty PassInfo,
                                    QuantumDialect DialectTy) {

  auto CommutedOps = performCommutation(analysis, PassInfo);

  for (auto Ops : CommutedOps) {

    auto *Op1 = Ops.Op1;
    auto *Op2 = Ops.Op2;
    auto funcOp = Op1->getParentOfType<mlir::func::FuncOp>();
    auto KernelDialectInfo = analysis.getKernelDialectInfo();
    assert(KernelDialectInfo.count(funcOp) &&
           "Get Op Info: No QuantumOpInfo for funcOp");
    auto Qview = KernelDialectInfo[funcOp].OpQViewMap;

    if (Qview.count(Op2)) {
      auto Op2TargetQubits = Qview[Op2].getQubits(QubitRole::Target).ids;
      assert(Op2TargetQubits.size() == 1 &&
             "Currently, Can only switch single qubit gates!");

      mlir::IRRewriter builder(Op2->getContext());
      builder.setInsertionPointAfter(Op2);

      // Since gate creation has dialect specific semantics, need to have
      //  dialect specific APIs to create the gates.
      //  TODO: 1. Can we do better? Do not like the use of MACROS
      //        2. What if Op1 is to be switched out and replaced?

      auto Op1Ty = Qview[Op1].GateTy;
      auto Op2Ty = Qview[Op2].GateTy;

      Operation *OpToSwitchOut = nullptr;
      StringRef ReplacementGateTy = "";
      for (auto &[Gatety, ReplaceWithTy] : PassInfo.ReplacementMap) {
        if (Gatety == Op1Ty) {
          OpToSwitchOut = Op1;
          ReplacementGateTy = parseGateTy(ReplaceWithTy);
          break;
        } else if (Gatety == Op2Ty) {
          OpToSwitchOut = Op2;
          ReplacementGateTy = parseGateTy(ReplaceWithTy);
          break;
        }
      }

      assert(OpToSwitchOut && "Op to switch out cannot be NULL!!");
      assert(!ReplacementGateTy.empty() && "Need the replacementgatety!!");

      auto TargetInQubitOps =
          Qview[OpToSwitchOut].getQubits(QubitRole::Target).in;

      auto *NewOp =
          createNewGate(OpToSwitchOut->getLoc(), DialectTy, ReplacementGateTy,
                        {}, TargetInQubitOps, {}, builder);

      analysis.addOperation(NewOp);

      OpToSwitchOut->replaceAllUsesWith(NewOp);
      // TODO: Should the operands of the following Ops be updated in
      // QuantumOpView?
      //       after replacement?

      builder.eraseOp(OpToSwitchOut);
    }
  }
}

static void performNullRotationCancellation(
    MapVector<Operation *, QuantumOpView> OpQuantumView,
    std::vector<Gate> GatesToCancel) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, FirstGateQView] : OpQuantumView) {

    auto GateTy = FirstGateQView.GateTy;

    if (!is_contained(GatesToCancel, GateTy)) {
      // It is assumed that GatesToCancel will contain : RX, RY, RZ
      continue;
    }
    auto parameters = FirstGateQView.params;
    bool deleteGate = true;
    for (auto param : parameters) {
      if (auto constOp = param.getDefiningOp<mlir::arith::ConstantOp>()) {
        if (auto floatAttr = dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double v = floatAttr.getValueAsDouble();
          if (!checkDoublePiMultiplies(v) && v != 0) {
            deleteGate = false;
            break;
          }
        }
      }
    }

    if (deleteGate)
      ToErase.insert(GateOp);
  }

  for (auto gop : ToErase) {
    MQSS_DEBUG("--->Erasing: " << *gop << "\n");
    cancel(gop);
  }
}

static void
performCancellation(MapVector<Operation *, QuantumOpView> OpQuantumView,
                    std::vector<Gate> GatesToCancel, Comparety CompareKey) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[FirstGateOp, FirstGateQView] : OpQuantumView) {

    auto FirstGateTy = FirstGateQView.GateTy;
    if (!FirstGateQView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
      continue;

    if (!is_contained(GatesToCancel, FirstGateTy))
      continue;

    if (ToErase.count(FirstGateOp))
      continue;

    auto *NextOp = FirstGateOp->getNextNode();
    if (!NextOp)
      continue;

    auto FirstGateCtrlQubits = FirstGateQView.getQubits(QubitRole::Control);
    auto FirstGateTargetQubits = FirstGateQView.getQubits(QubitRole::Target);

    while (NextOp) {

      auto nextOpView = OpQuantumView[NextOp];
      auto nextGateCtrlQubits = nextOpView.getQubits(QubitRole::Control);
      auto nextGateTargetQubits = nextOpView.getQubits(QubitRole::Target);
      // TODO: Should the "touchesAny" check be there?
      if (nextOpView.hasSideEffects && (nextOpView.GateTy != FirstGateTy)) {
        if (touchesAny(NextOp, FirstGateCtrlQubits.ids, OpQuantumView) ||
            touchesAny(NextOp, FirstGateTargetQubits.ids, OpQuantumView)) {
          break;
        }
        if (touchesAny(NextOp, FirstGateTargetQubits.out)) {
          break;
        }
      }

      if (nextOpView.GateTy == FirstGateTy) {

        if (SameQubits({FirstGateCtrlQubits.ids, FirstGateTargetQubits.ids},
                       {nextGateCtrlQubits.ids, nextGateTargetQubits.ids},
                       CompareKey)) {
          ToErase.insert(FirstGateOp);
          ToErase.insert(NextOp);
          NextOp = NextOp->getNextNode();
        } else if (SameQubitValues(
                       {FirstGateCtrlQubits.out, FirstGateTargetQubits.out},
                       {nextGateCtrlQubits.in, nextGateTargetQubits.in},
                       CompareKey)) {
          ToErase.insert(FirstGateOp);
          ToErase.insert(NextOp);
          NextOp = NextOp->getNextNode();
        }

        break;
      }

      NextOp = NextOp->getNextNode();
    }
  }

  for (auto *op : ToErase) {
    MQSS_DEBUG("--> Erasing: " << *op << "\n");

    for (unsigned i = 0; i < op->getNumResults(); ++i) {
      op->getResult(i).replaceAllUsesWith(op->getOperand(i));
    }

    cancel(op);
  }
}

struct pipelinety {
  Operation *GateOp;
  QuantumOpView GateOpView;
};

static void performReduction(MyModuleAnalysis &analysis,
                             const ReductionPassInfoty PassInfo,
                             QuantumDialect DialectTy) {

  if (PassInfo.NewGateTy == Gate::UNKNOWN) {
    return;
  }

  int i = 0;
  auto FirstGateToCancelTy = PassInfo.GatesToCancel[i];

  for (auto &[Kernel, KernelInfo] : analysis.getKernelDialectInfo()) {

    if (KernelInfo.AllocatedQubits == 0)
      continue;

    std::vector<pipelinety> pipeline;
    std::vector<std::vector<pipelinety>> ToErase;
    auto OpQuantumView = KernelInfo.OpQViewMap;
    for (auto &[GateOp, FirstGateQView] : OpQuantumView) {

      auto FirstGateTy = FirstGateQView.GateTy;
      if (!FirstGateQView.hasSideEffects || FirstGateTy == Gate::UNKNOWN)
        continue;

      if (is_contained(PassInfo.GatesToCancel, FirstGateTy) &&
          pipeline.size() != PassInfo.GatesToCancel.size()) {
        pipeline.push_back({GateOp, FirstGateQView});
      }

      if (pipeline.size() == PassInfo.GatesToCancel.size()) {
        bool patterncheck = true;
        for (auto i = 0; i < pipeline.size(); i++) {
          if (pipeline[i].GateOpView.GateTy != PassInfo.GatesToCancel[i]) {
            break;
          }
        }

        if (!patterncheck) {
          pipeline.clear();
          continue;
        }

        bool ChecksSatisfied = false;
        for (int i = 0; i < pipeline.size() - 1; i++) {
          auto FirstPatternOp = pipeline[i].GateOp;
          auto FirstPatternOpQView = pipeline[i].GateOpView;
          auto SecondPatternOp = pipeline[i + 1].GateOp;
          auto SecondPatternOpQView = pipeline[i].GateOpView;

          auto NextOp = FirstPatternOp->getNextNode();

          bool InterveningOps = false;
          auto FirstGateCtrlQubits =
              FirstPatternOpQView.getQubits(QubitRole::Control);
          auto FirstGateTargetQubits =
              FirstPatternOpQView.getQubits(QubitRole::Target);

          auto SecondGateCtrlQubits =
              SecondPatternOpQView.getQubits(QubitRole::Control);
          auto SecondGateTargetQubits =
              SecondPatternOpQView.getQubits(QubitRole::Target);

          while (NextOp != SecondPatternOp) {
            auto nextOpView = OpQuantumView[NextOp];
            if (nextOpView.hasSideEffects) {
              if (touchesAny(NextOp, FirstGateCtrlQubits.ids, OpQuantumView) ||
                  touchesAny(NextOp, FirstGateTargetQubits.ids,
                             OpQuantumView)) {
                InterveningOps = true;
                break;
              }
              auto FirstGateTargetOutQubits =
                  FirstGateQView.getQubits(QubitRole::Target).out;
              if (touchesAny(NextOp, FirstGateTargetOutQubits)) {
                InterveningOps = true;
                break;
              }
            }

            NextOp = NextOp->getNextNode();
          }

          if (InterveningOps) {
            ChecksSatisfied = false;
            MQSS_DEBUG("Has intervening ops!\n");
            break;
          }

          if (SameQubits({FirstGateCtrlQubits.ids, FirstGateTargetQubits.ids},
                         {SecondGateCtrlQubits.ids, SecondGateTargetQubits.ids},
                         PassInfo.CompareKey)) {
            ChecksSatisfied = true;
            break;
          }
          if (SameQubitValues(
                  {FirstGateCtrlQubits.out, FirstGateTargetQubits.out},
                  {SecondGateCtrlQubits.in, SecondGateTargetQubits.in},
                  PassInfo.CompareKey)) {
            ChecksSatisfied = true;
            break;
          }
          MQSS_DEBUG(*pipeline[i].GateOp << "\n");
          MQSS_DEBUG(*pipeline[i + 1].GateOp << "\n");

          // TODO: For catalyst - Check equivalence b/w result Qubits of FirstOp
          // and OpQubits of Next Op
          //      See performCancellation for reference
        }
        if (ChecksSatisfied) {
          ToErase.push_back(pipeline);
        }
        pipeline.clear();
      }
    }

    std::vector<mlir::Value> QubitsbaseVector;

    for (auto pattern : ToErase) {

      // TODO: Is it correct to take this as the RefOp?
      auto FirstOp = pattern[0].GateOp;
      MQSS_DEBUG("Ref gate: " << *FirstOp << "\n");

      mlir::IRRewriter builder(FirstOp->getContext());
      builder.setInsertionPointAfter(FirstOp);

      auto RefOpParams = OpQuantumView[FirstOp].params;

      auto RefOpCtrlQubits =
          OpQuantumView[FirstOp].getQubits(QubitRole::Control).in;
      auto RefOptargetQubits =
          OpQuantumView[FirstOp].getQubits(QubitRole::Target).in;

      auto *NewOp = createNewGate(
          FirstOp->getLoc(), DialectTy, parseGateTy(PassInfo.NewGateTy),
          RefOpCtrlQubits, RefOptargetQubits, RefOpParams, builder);

      assert(NewOp && "Replacement Gate not created!");
      analysis.addOperation(NewOp);
      auto FinalOp = pattern[pattern.size() - 1].GateOp;
      if (!FinalOp->getUsers().empty()) {
        // TODO: Should be revisited!
        auto NewOpResults = analysis.getOpInfo(NewOp)
                                .getQubits(PassInfo.CompareKey.KeyGate2)
                                .out[0];
        FinalOp->replaceAllUsesWith(NewOp);
      } else {
        for (auto entry : pattern) {
          cancel(entry.GateOp);
        }
      }

      MQSS_DEBUG("--->Created: " << *NewOp << "\n");
      // for (auto &[Op, view] : pattern) {
      //    "-->To Erase: " << *Op << "\n";
      //   cancel(Op);
      // }
    }
  }
}

static void performArgAngelNormalization(
    MapVector<Operation *, QuantumOpView> OpQuantumView,
    QuantumDialect DialectTy) {

  SmallSetVector<Operation *, 16> ToErase;
  for (auto &[GateOp, GateQView] : OpQuantumView) {
    if (GateQView.GateTy != Gate::RX && GateQView.GateTy != Gate::RY &&
        GateQView.GateTy != Gate::RZ) {
      continue;
    }

    mlir::IRRewriter builder(GateOp->getContext());
    builder.setInsertionPointAfter(GateOp);

    std::vector<mlir::Value> nParameters;
    for (auto param : GateQView.params) {
      if (auto constOp = param.getDefiningOp<mlir::arith::ConstantOp>()) {
        if (auto floatAttr = dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double v = floatAttr.getValueAsDouble();

          auto newVal = normalizeValue(v, builder, GateOp->getLoc());

          if (newVal != param)
            nParameters.push_back(newVal);
        }
      }
    }

    if (nParameters.empty())
      continue;

    ToErase.insert(GateOp);
    ValueRange normalizedParams(nParameters);

    auto GateCtrlQubits = GateQView.getQubits(QubitRole::Control).in;
    auto GateTargetQubits = GateQView.getQubits(QubitRole::Target).in;

    auto *NewOp = createNewGate(GateOp->getLoc(), DialectTy,
                                parseGateTy(GateQView.GateTy), GateCtrlQubits,
                                GateTargetQubits, GateQView.params, builder,
                                GateQView.isAdjoint);

    MQSS_DEBUG("--->Created: " << *NewOp << "\n");
  }

  for (auto *Op : ToErase) {
    MQSS_DEBUG("-->To Erase: " << *Op << "\n");
    cancel(Op);
  }
}

// Performs CNOT Reversal, with SSA resolution (For value semantics)
// For the following example input pattern:
//   %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits, %out_qubits_0 :
//   !quantum.bit, !quantum.bit %out_qubits_2 = quantum.custom "PauliY"()
//   %out_qubits_1#0 : !quantum.bit %out_qubits_3 = quantum.custom "PauliY"()
//   %out_qubits_1#1 : !quantum.bit
// Following the SSA resolution:
//    %h1_ctrl = quantum.custom "Hadamard"() %out_qubits : !quantum.bit
//    %h1_tgt = quantum.custom "Hadamard"() %out_qubits_0 : !quantum.bit
//    %cnot_out:2 = quantum.custom "CNOT"() %h1_ctrl, %h1_tgt : !quantum.bit,
//    !quantum.bit %h2_ctrl = quantum.custom "Hadamard"() %cnot_out#0 :
//    !quantum.bit %h2_tgt = quantum.custom "Hadamard"() %cnot_out#1 :
//    !quantum.bit %out_qubits_2 = quantum.custom "PauliY"() %h2_ctrl :
//    !quantum.bit %out_qubits_3 = quantum.custom "PauliY"() %h2_tgt :
//    !quantum.bit
static void performCNOTReversal(MyModuleAnalysis &analysis,
                                QuantumDialect DialectTy) {

  SmallSetVector<Operation *, 16> ToErase;
  std::unordered_map<Operation *, std::vector<Operation *>> DecomposeMap;
  for (auto &[kernel, kernelInfo] : analysis.getKernelDialectInfo()) {
    if (kernelInfo.AllocatedQubits == 0)
      continue;

    auto OpQuantumView = kernelInfo.OpQViewMap;
    for (auto &[GateOp, GateQView] : OpQuantumView) {
      if ((GateQView.GateTy != Gate::CNOT))
        continue;

      mlir::IRRewriter builder(GateOp->getContext());
      builder.setInsertionPointAfter(GateOp);

      std::vector<Operation *> NewPattern;
      auto ControlInQubitOps = GateQView.getQubits(QubitRole::Control).in;
      auto TargetInQubitOps = GateQView.getQubits(QubitRole::Target).in;
      auto loc = GateOp->getLoc();
      auto NewGate1 = createNewGate(loc, DialectTy, "H", {}, TargetInQubitOps,
                                    GateQView.params, builder);

      auto NewGate2 = createNewGate(loc, DialectTy, "H", {}, ControlInQubitOps,
                                    GateQView.params, builder);

      auto NewGate3 =
          createNewGate(loc, DialectTy, "CNOT", TargetInQubitOps,
                        ControlInQubitOps, GateQView.params, builder);

      auto NewGate4 = createNewGate(loc, DialectTy, "H", {}, TargetInQubitOps,
                                    GateQView.params, builder);

      auto NewGate5 = createNewGate(loc, DialectTy, "H", {}, ControlInQubitOps,
                                    GateQView.params, builder);

      analysis.addOperation(NewGate1);
      analysis.addOperation(NewGate2);
      analysis.addOperation(NewGate3);
      analysis.addOperation(NewGate4);
      analysis.addOperation(NewGate5);

      DecomposeMap[GateOp] = {NewGate1, NewGate2, NewGate3, NewGate4, NewGate5};
    }
  }

  for (auto &[Op, DecomposePattern] : DecomposeMap) {
    auto Gate1 = DecomposePattern[0];
    auto Gate2 = DecomposePattern[1];
    auto Gate3 = DecomposePattern[2];
    auto Gate4 = DecomposePattern[3];
    auto Gate5 = DecomposePattern[4];
    auto OpQview = analysis.getOpInfo(Op);

    if (OpQview.getQubits(QubitRole::Target).out.empty() &&
        OpQview.getQubits(QubitRole::Control).out.empty())
      continue;

    auto Gate1Tgt =
        analysis.getOpInfo(Gate1).getQubits(QubitRole::Target).out[0];
    auto Gate2Tgt =
        analysis.getOpInfo(Gate2).getQubits(QubitRole::Target).out[0];

    auto Gate3OpInfo = analysis.getOpInfo(Gate3);
    auto Gate3CtrlOp = Gate3OpInfo.getQubits(QubitRole::Control).in[0];
    auto Gate3TgtOp = Gate3OpInfo.getQubits(QubitRole::Target).in[0];

    MQSS_DEBUG("Gate3 Op Info fine\n");
    auto Gate3ResultCtrl = Gate3OpInfo.getQubits(QubitRole::Control).out[0];
    auto Gate3ResultTgt = Gate3OpInfo.getQubits(QubitRole::Target).out[0];

    MQSS_DEBUG("Gate3 Result Info fine\n");
    auto Gate4OpInfo = analysis.getOpInfo(Gate4);
    auto Gate5OpInfo = analysis.getOpInfo(Gate5);
    auto Gate4OpTgt = Gate4OpInfo.getQubits(QubitRole::Target).in[0];
    auto Gate5OpTgt = Gate5OpInfo.getQubits(QubitRole::Target).in[0];
    MQSS_DEBUG("Gate4 and 5 Target Info fine\n");

    Gate3->replaceUsesOfWith(Gate3CtrlOp, Gate1Tgt);
    Gate3->replaceUsesOfWith(Gate3TgtOp, Gate2Tgt);
    Gate4->replaceUsesOfWith(Gate4OpTgt, Gate3ResultCtrl);
    Gate5->replaceUsesOfWith(Gate5OpTgt, Gate3ResultTgt);

    auto OpResultCtrl = OpQview.getQubits(QubitRole::Control).out[0];
    auto OpResultTgt = OpQview.getQubits(QubitRole::Target).out[0];

    auto Gate4ResultTgt = Gate4OpInfo.getQubits(QubitRole::Target).out[0];
    auto Gate5ResultTgt = Gate5OpInfo.getQubits(QubitRole::Target).out[0];

    OpResultCtrl.replaceAllUsesWith(Gate4ResultTgt);
    OpResultTgt.replaceAllUsesWith(Gate5ResultTgt);

    ToErase.insert(Op);
  }

  for (auto *Op : ToErase) {
    MQSS_DEBUG("-->To Erase: " << *Op << "\n");
    cancel(Op);
  }
}
