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

#include "Passes/CodeGen/BasisConversionPatterns.h"
#include "Passes/CodeGen/CodeGenPasses.h"

#include <llvm/ADT/StringSet.h>

namespace mqssci::codegen {

#define GEN_PASS_DEF_BASISCONVERSIONPASS
#include "Passes/CodeGen/CodeGen.h.inc"

} // namespace mqssci::codegen

using namespace mlir;

namespace {

const std::unordered_map<std::string, std::vector<DecompositionRule>> &
getDecompositionTable() {
  static const std::unordered_map<std::string, std::vector<DecompositionRule>>
      table = [] {
        std::unordered_map<std::string, std::vector<DecompositionRule>> t;
        t["h"] = {
            makeRule<quake::HOp>("HToRzXRz", {"rz", "x"}, rewriteHToRzXRz),
            makeRule<quake::HOp>("HToU3", {"u3"}, rewriteHToU3),
            makeRule<quake::HOp>("HToPhasedRx", {"phased_rx"},
                                 rewriteHToPhasedRx)};
        t["x"] = {makeRule<quake::XOp>("XToHZH", {"h", "z"}, rewriteXToHZH),
                  makeRule<quake::XOp>("XToRx", {"rx"}, rewriteXToRx),
                  makeRule<quake::XOp>("XToPhasedRx", {"phased_rx"},
                                       rewriteXToPhasedRx)};
        t["y"] = {makeRule<quake::YOp>("YToRy", {"ry"}, rewriteYToRy),
                  makeRule<quake::YOp>("YToPhasedRx", {"phased_rx"},
                                       rewriteYToPhasedRx)};
        t["z"] = {makeRule<quake::ZOp>("ZToHXH", {"h", "x"}, rewriteZToHXH),
                  makeRule<quake::ZOp>("ZToRz", {"rz"}, rewriteZToRz)};
        t["s"] = {
            makeRule<quake::SOp>("SToRz", {"rz"}, rewriteSToRz),
            makeRule<quake::SOp>("SToSdgSdgSdg", {"sdg"}, rewriteSToSdgSdgSdg),
            makeRule<quake::SOp>("SToTT", {"t"}, rewriteSToTT),
            makeRule<quake::SOp>("SToPhasedRx", {"phased_rx"},
                                 rewriteSToPhasedRx)};
        t["sdg"] = {makeRule<quake::SOp>("SdgToRz", {"rz"}, rewriteSdgToRz),
                    makeRule<quake::SOp>("SdgToSSS", {"s"}, rewriteSdgToSSS)};
        t["t"] = {makeRule<quake::TOp>("TToRz", {"rz"}, rewriteTToRz),
                  makeRule<quake::TOp>("TToPhasedRx", {"phased_rx"},
                                       rewriteTToPhasedRx)};
        t["tdg"] = {makeRule<quake::TOp>("TdgToRz", {"rz"}, rewriteTdgToRz)};
        t["r1"] = {makeRule<quake::R1Op>("R1ToRz", {"rz"}, rewriteR1ToRz),
                   makeRule<quake::R1Op>("R1ToPhasedRx", {"phased_rx"},
                                         rewriteR1ToPhasedRx)};
        t["rx"] = {
            makeRule<quake::RxOp>("RxToHRzH", {"h", "rz"}, rewriteRxToHRzH),
            makeRule<quake::RxOp>("RxToPhasedRx", {"phased_rx"},
                                  rewriteRxToPhasedRx)};
        t["ry"] = {makeRule<quake::RyOp>("RyToRzRxRz", {"rz", "rx"},
                                         rewriteRyToRzRxRz)};
        t["rz"] = {
            makeRule<quake::RzOp>("RzToHRxH", {"h", "rx"}, rewriteRzToHRxH),
            makeRule<quake::RzOp>("RzToU3", {"u3"}, rewriteRzToU3),
            makeRule<quake::RzOp>("RzToPhasedRx", {"phased_rx"},
                                  rewriteRzToPhasedRx)};
        t["u2"] = {makeRule<quake::U2Op>("U2ToRzRyRz", {"rz", "ry"},
                                         rewriteU2ToRzRyRz),
                   makeRule<quake::U2Op>("U2ToU3", {"u3"}, rewriteU2ToU3)};
        t["u3"] = {makeRule<quake::U3Op>("U3ToRzRyRz", {"rz", "ry"},
                                         rewriteU3ToRzRyRz)};
        t["swap"] = {makeRule<quake::SwapOp>("SwapToUpperCxCxCx", {"cx"},
                                             rewriteSwapToUpperCxCxCx)};
        t["cx"] = {makeRule<quake::XOp>("CxToUpperHCzH", {"h", "cz"},
                                        rewriteCxToUpperHCzH)};
        t["cz"] = {makeRule<quake::ZOp>("CzToUpperHCxH", {"h", "cx"},
                                        rewriteCzToUpperHCxH)};
        t["cy"] = {makeRule<quake::YOp>("CyToSCxSdg", {"s", "cx", "sdg"},
                                        rewriteCyToSCxSdg)};
        t["crx"] = {makeRule<quake::RxOp>("CrxToHCrzH", {"h", "crz"},
                                          rewriteCrxToHCrzH)};
        t["crz"] = {makeRule<quake::RzOp>("CrzToHCrxH", {"h", "crx"},
                                          rewriteCrzToHCrxH),
                    makeRule<quake::RzOp>("CrzToRzCu3", {"rz", "u3"},
                                          rewriteCrzToRzCu3),
                    makeRule<quake::RzOp>("CrzToRzCxRzCx", {"rz", "cx"},
                                          rewriteCrzToRzCxRzCx)};
        t["cry"] = {makeRule<quake::RyOp>("CryToRzCrxRz", {"rz", "crx"},
                                          rewriteCryToRzCrxRz)};
        return t;
      }();
  return table;
}

//===----------------------------------------------------------------------===//
// Classification: maps an Operation* to the native-gate-set mnemonic it
// represents, or std::nullopt if this pass doesn't recognize its shape
// (wrong number of controls/targets/parameters, or an adjoint rotation --
// none of the rules above handle adjoint Rx/Ry/Rz/R1, matching the scope of
// the individual passes they were copied from).
//===----------------------------------------------------------------------===//

std::optional<std::string> classifyOp(Operation *op) {
  if (auto g = dyn_cast<quake::HOp>(op)) {
    if (g.getControls().empty() && g.getTargets().size() == 1)
      return std::string("h");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::XOp>(op)) {
    if (g.getTargets().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("x");
    if (g.getControls().size() == 1)
      return std::string("cx");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::YOp>(op)) {
    if (g.isAdj() || g.getTargets().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("y");
    if (g.getControls().size() == 1)
      return std::string("cy");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::ZOp>(op)) {
    if (g.getTargets().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("z");
    if (g.getControls().size() == 1)
      return std::string("cz");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::SOp>(op)) {
    if (!g.getControls().empty() || g.getTargets().size() != 1)
      return std::nullopt;
    return std::string(g.isAdj() ? "sdg" : "s");
  }
  if (auto g = dyn_cast<quake::TOp>(op)) {
    if (!g.getControls().empty() || g.getTargets().size() != 1)
      return std::nullopt;
    return std::string(g.isAdj() ? "tdg" : "t");
  }
  if (auto g = dyn_cast<quake::R1Op>(op)) {
    if (g.isAdj() || !g.getControls().empty() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 1)
      return std::nullopt;
    return std::string("r1");
  }
  if (auto g = dyn_cast<quake::RxOp>(op)) {
    if (g.isAdj() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("rx");
    if (g.getControls().size() == 1)
      return std::string("crx");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::RyOp>(op)) {
    if (g.isAdj() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("ry");
    if (g.getControls().size() == 1)
      return std::string("cry");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::RzOp>(op)) {
    if (g.isAdj() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 1)
      return std::nullopt;
    if (g.getControls().empty())
      return std::string("rz");
    if (g.getControls().size() == 1)
      return std::string("crz");
    return std::nullopt;
  }
  if (auto g = dyn_cast<quake::U2Op>(op)) {
    if (!g.getControls().empty() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 2)
      return std::nullopt;
    return std::string("u2");
  }
  if (auto g = dyn_cast<quake::U3Op>(op)) {
    if (!g.getControls().empty() || g.getTargets().size() != 1 ||
        g.getParameters().size() != 3)
      return std::nullopt;
    return std::string("u3");
  }
  if (auto g = dyn_cast<quake::SwapOp>(op)) {
    if (!g.getControls().empty() || g.getTargets().size() != 2)
      return std::nullopt;
    return std::string("swap");
  }
  if (auto g = dyn_cast<quake::PhasedRxOp>(op)) {
    if (!g.isAdj() && g.getControls().empty() &&
        g.getParameters().size() == 2 && g.getTargets().size() == 1)
      return std::string("phased_rx");
    return std::nullopt;
  }
  return std::nullopt;
}

// For a given native set, figures out -- for every mnemonic that isn't
// itself native -- one rule that is *transitively* guaranteed to bottom out
// in native gates (not just a rule whose immediate output happens to be
// native already). This is a fixed-point over the whole table: a mnemonic is
// "resolvable" if native, or if some rule for it produces only mnemonics
// that are themselves resolvable.
//
// This is still not a cost model (it doesn't compare resolvable rules by
// depth/gate count, it just takes the first resolvable one found), but
// skipping it and only doing a one-hop lookahead is actively wrong: e.g. for
// native = {h, rx, cx}, "crz" has no rule whose *immediate* output is fully
// native, so a one-hop check always falls back to CrzToHCrxH -- which only
// ever produces "crx", whose only rule (CrxToHCrzH) produces "crz" right
// back. That pair bounces forever, and because each round wraps another H on
// each side without ever cancelling the previous ones, the circuit grows
// without bound instead of converging (confirmed by direct testing). The
// fixed point below instead finds that CrzToRzCxRzCx -> {rz, cx} works,
// because rz is itself resolvable via RzToHRxH -> {h, rx}.
std::unordered_map<std::string, const DecompositionRule *> computeWitnesses(
    const std::unordered_map<std::string, std::vector<DecompositionRule>>
        &table,
    const llvm::StringSet<> &native) {
  std::unordered_map<std::string, const DecompositionRule *> witness;
  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto &entry : table) {
      const std::string &mnemonic = entry.first;
      if (native.contains(mnemonic) || witness.count(mnemonic))
        continue;
      for (const auto &rule : entry.second) {
        bool allResolvable = true;
        for (const auto &produced : rule.produces) {
          if (native.contains(produced) || witness.count(produced))
            continue;
          allResolvable = false;
          break;
        }
        if (allResolvable) {
          witness[mnemonic] = &rule;
          changed = true;
          break;
        }
      }
    }
  }
  return witness;
}

struct BasisConversion
    : public mqssci::codegen::impl::BasisConversionPassBase<BasisConversion> {
public:
  BasisConversion() = default;

  explicit BasisConversion(const BasisConversionPassOptions &options)
      : BasisConversion() {
    gates = options.gates;
  }

  void runOnOperation() override {
    bool wasApplied = false;

    auto kernel = getOperation();
    llvm::StringSet<> native;
    auto split_gates = split(gates, ",");
    for (const auto &gate : split_gates)
      native.insert(gate);

    const auto &table = getDecompositionTable();
    // Computed once, up front: for every non-native mnemonic that can
    // transitively bottom out in native gates, the rule that gets it there.
    // Mnemonics absent from this map (but present in the table) are known
    // gates this pass genuinely cannot legalize into the requested set --
    // warned about once below, and left untouched, rather than repeatedly
    // rewritten with a rule that can never converge.
    const auto witnesses = computeWitnesses(table, native);
    llvm::StringSet<> warnedUnresolvable;

    // Applying a witness rule can itself introduce ops whose own witness
    // rule hasn't fired yet (e.g. crz -> {rz, cx}, then rz -> {h, rx}), so
    // this still needs multiple rounds; the loop is now just mechanically
    // unwinding an acyclic chain, so it always reaches a fixed point in at
    // most table.size() rounds. The cap is only a defensive backstop.
    constexpr int maxRounds = 64;
    for (int round = 0; round < maxRounds; ++round) {
      bool changed = false;
      kernel.walk([&](Operation *op) {
        std::optional<std::string> mnemonic = classifyOp(op);
        if (!mnemonic || native.contains(*mnemonic))
          return;
        auto wit = witnesses.find(*mnemonic);
        if (wit == witnesses.end()) {
          if (table.count(*mnemonic) &&
              warnedUnresolvable.insert(*mnemonic).second)
            mlir::emitWarning(kernel.getLoc())
                << "BasisConversion Op: '" << *mnemonic
                << "' cannot be legalized, leaving it as-is...";
          return;
        }
        IRRewriter rewriter(op->getContext());
        wit->second->apply(rewriter, op);
        changed = true;
        wasApplied = true;
      });
      if (!changed)
        return;
      if (round == maxRounds - 1)
        mlir::emitWarning(kernel.getLoc())
            << "BasisConversion: gave up after " << maxRounds
            << " rounds without reaching a fixed point";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqssci::codegen::BasisConversionPass() {
  return std::make_unique<BasisConversion>();
}

std::unique_ptr<Pass> mqssci::codegen::createBasisConversionPass(
    const BasisConversionPassOptions &options) {
  return std::make_unique<BasisConversion>(options);
}
