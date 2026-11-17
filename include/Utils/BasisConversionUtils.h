

#pragma once

#include "Passes/Analysis/DialectAnalysisSelector.h"
#include "Utils/DebugUtils.h"
#include "Utils/DialectUtils.h"

#include <unordered_map>

// Replace `op` with the gates created since the rule started, restoring
// linear qubit SSA for value-semantic dialects. Quake (no results): just erase.
bool replaceGateResolvingSSA(MyModuleAnalysis &analysis, IRRewriter &rewriter,
                             Operation *op, const QuantumOpView &view,
                             std::vector<Operation *> newGates) {

  if (!view.hasValueSemantics()) {
    rewriter.eraseOp(op);
    return true;
  }

  llvm::DenseMap<mlir::Value, mlir::Value> latest_table;
  std::vector<mlir::Value> oldOut;
  std::vector<mlir::Value> oldIn;

  for (auto const &op : view.getQubits(QubitRole::Control).in) {
    latest_table[op] = op;
    oldIn.emplace_back(op);
  }
  for (auto const &op : view.getQubits(QubitRole::Target).in) {
    latest_table[op] = op;
    oldIn.emplace_back(op);
  }
  for (auto const &op : view.getQubits(QubitRole::Control).out) {
    oldOut.emplace_back(op);
  }
  for (auto const &op : view.getQubits(QubitRole::Target).out) {
    oldOut.emplace_back(op);
  }

  for (auto &newGate : newGates) {
    const auto &new_gate_view = analysis.getOpInfo(newGate);
    if (!new_gate_view.has_value()) {
      mlir::emitError(
          newGate->getLoc(),
          "BasisConversion: ResolvingSSA: New Op does not have QuantumOpView");
      return false;
    }

    // Need to use a copy of the operands here
    auto new_gate_in_ctrl_ops = new_gate_view->getQubits(QubitRole::Control).in;
    auto new_gate_out_ctrl_ops =
        new_gate_view->getQubits(QubitRole::Control).out;

    auto new_gate_in_tgt_ops = new_gate_view->getQubits(QubitRole::Target).in;
    auto new_gate_out_tgt_ops = new_gate_view->getQubits(QubitRole::Target).out;

    for (unsigned i = 0; i < new_gate_in_ctrl_ops.size(); ++i) {
      if (!latest_table.count(new_gate_in_ctrl_ops[i])) {
        mlir::emitError(new_gate_in_ctrl_ops[i].getLoc(),
                        "BasisConversion: ResolvingSSA: Operands "
                        "of newly created gate not tracked in Operand table");
        return false;
      }
      if (new_gate_in_ctrl_ops[i] != latest_table[new_gate_in_ctrl_ops[i]]) {
        rewriter.modifyOpInPlace(newGate, [&] {
          newGate->replaceUsesOfWith(
              new_gate_in_ctrl_ops[i],
              latest_table[new_gate_in_ctrl_ops[i]]); // this op's operands only
        });
      }

      latest_table[new_gate_in_ctrl_ops[i]] = new_gate_out_ctrl_ops[i];
    }

    for (unsigned i = 0; i < new_gate_in_tgt_ops.size(); ++i) {

      if (!latest_table.count(new_gate_in_tgt_ops[i])) {
        mlir::emitError(new_gate_in_tgt_ops[i].getLoc(),
                        "BasisConversion: ResolvingSSA: Operands "
                        "of newly created gate not tracked in Operand table");
        return false;
      }

      if (new_gate_in_tgt_ops[i] != latest_table[new_gate_in_tgt_ops[i]]) {

        rewriter.modifyOpInPlace(newGate, [&] {
          newGate->replaceUsesOfWith(
              new_gate_in_tgt_ops[i],
              latest_table[new_gate_in_tgt_ops[i]]); // this op's operands only
        });
      }

      latest_table[new_gate_in_tgt_ops[i]] = new_gate_out_tgt_ops[i];
    }
  }

  // Finish: for each position j, call rewriter.replaceAllUsesWith(oldOut[j],
  // latest[oldIn[j]]). Then rewriter.eraseOp(op).

  for (unsigned j = 0; j < oldOut.size(); j++) {
    rewriter.replaceAllUsesWith(oldOut[j], latest_table[oldIn[j]]);
  }

  if (!op->use_empty()) {

    mlir::emitError(op->getLoc(), "BasisConversion: ResolvingSSA: Cannot Erase "
                                  "Op that has uses remaining!");
    return false;
  }

  rewriter.eraseOp(op);
  return true;
}
