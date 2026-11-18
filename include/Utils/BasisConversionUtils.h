

#pragma once

#include "Passes/Analysis/DialectAnalysisSelector.h"
#include "Utils/DebugUtils.h"
#include "Utils/DialectUtils.h"

#include <unordered_map>

// Replaces the gate `op` by `newGates` (the gates a decomposition rule just
// created after it, in creation order) and restores qubit SSA for
// value-semantic dialects such as Catalyst, where every gate consumes qubit
// values and produces new ones.
//
// The rules build every new gate from the ORIGINAL operands of `op`. Here the
// new gates are rewired into a chain: each takes the latest value of every
// qubit it touches, and the users of `op`'s results get the final values.
// E.g. cx(a, b) -> h(b); cz(a, b); h(b) becomes
//   b1 = h(b);  a1, b2 = cz(a, b1);  b3 = h(b2)
// and the users of the cx results now use a1 and b3.
//
// Quake has reference semantics (gates have no results): `op` is just erased.
bool replaceGateResolvingSSA(MyModuleAnalysis &analysis, IRRewriter &rewriter,
                             Operation *op, const QuantumOpView &view,
                             std::vector<Operation *> newGates) {

  if (!view.hasValueSemantics()) {
    rewriter.eraseOp(op);
    return true;
  }

  // latest_table: original operand of `op` -> newest value standing for that
  // qubit so far. Starts as the identity and advances as new gates are visited.
  // oldIn[j] and oldOut[j] are the same qubit (same position).
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

  // Visit the new gates in creation order; each still uses original operands.
  for (auto &newGate : newGates) {
    const auto &new_gate_view = analysis.getOpInfo(newGate);
    if (!new_gate_view.has_value()) {
      mlir::emitError(
          newGate->getLoc(),
          "BasisConversion: ResolvingSSA: New Op does not have QuantumOpView");
      return false;
    }

    // Copies: editing newGate below refreshes its cached view, and `in` must
    // keep holding the original operands (the keys of latest_table).
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
      // Point this gate at the latest value of the qubit. Only this op's
      // operand is edited: replaceAllUsesWith would also rewrite the earlier
      // new gates. modifyOpInPlace keeps the analysis in sync.
      if (new_gate_in_ctrl_ops[i] != latest_table[new_gate_in_ctrl_ops[i]]) {
        rewriter.modifyOpInPlace(newGate, [&] {
          newGate->replaceUsesOfWith(
              new_gate_in_ctrl_ops[i],
              latest_table[new_gate_in_ctrl_ops[i]]); // this op's operands only
        });
      }

      // This gate's result is now the latest value of that qubit.
      latest_table[new_gate_in_ctrl_ops[i]] = new_gate_out_ctrl_ops[i];
    }

    // Same for the target qubits.
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

  // Users of the old gate's results now take the final value of the same qubit.
  // (An empty decomposition leaves the original operand: a plain bypass.)

  for (unsigned j = 0; j < oldOut.size(); j++) {
    rewriter.replaceAllUsesWith(oldOut[j], latest_table[oldIn[j]]);
  }

  // Every result must have been replaced above, or erasing `op` is invalid.
  if (!op->use_empty()) {

    mlir::emitError(op->getLoc(), "BasisConversion: ResolvingSSA: Cannot Erase "
                                  "Op that has uses remaining!");
    return false;
  }

  rewriter.eraseOp(op);
  return true;
}
