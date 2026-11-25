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

#include "Passes/Analysis/Extractor.h"
#include "Passes/CodeGen/BasisConversionPatterns.h"
#include "Passes/CodeGen/CodeGenPasses.h"
#include "Utils/DebugUtils.h"
#include "Utils/QDMI_Utils.h"

#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Error.h>
#include <mlir/Transforms/GreedyPatternRewriteDriver.h>
#include <unordered_map>
#include <unordered_set>

namespace mqss::mqssci::codegen {

#define GEN_PASS_DEF_BASISCONVERSIONPASS
#include "Passes/CodeGen/CodeGen.h.inc"

} // namespace mqss::mqssci::codegen

using namespace mlir;

namespace {
class AnalysisSyncListener : public mlir::RewriterBase::Listener {
public:
  explicit AnalysisSyncListener(MyModuleAnalysis &analysis)
      : analysis(analysis) {}

  void notifyOperationInserted(Operation *op,
                               OpBuilder::InsertPoint previous) override {
    if (previous.isSet()) { // a move, not a new op: already in the map
      return;
    }
    if (!analysis.addOperation(
            op)) { // callbacks return void, so record failure
      failed = true;
      return;
    }

    if (recording && hasQubitOperands(op)) {
      recordedGates.push_back(op);
    }
  }

  void notifyOperationErased(Operation *op) override {
    analysis.removeOperation(op); // the new API; must not touch the op after
  }
  void notifyOperationModified(Operation *op) override {
    analysis.refreshOperation(op); // new API
  }

  // Recording is scoped to ONE rule application: call startRecording() right
  // before applying a rule and stopRecording() right after it. The result is
  // the qubit-consuming ops (gates) that rule created, in creation order --
  // not constants, and not anything inserted outside that window (e.g. by
  // the greedy folding between rounds).
  void startRecording() {
    recordedGates.clear();
    recording = true;
  }

  [[nodiscard]] std::vector<Operation *> stopRecording() {
    recording = false;
    std::vector<Operation *> gates;
    gates.swap(recordedGates);
    return gates;
  }

  bool failed = false;

private:
  // True if the analysis tracks `op` and it has control/target qubit operands
  // (i.e. it is a gate-like op, not a constant). Looks the view up directly
  // instead of calling getOpInfo(), which emits an error for untracked ops.
  bool hasQubitOperands(Operation *op) {
    auto func = op->getParentOfType<mlir::func::FuncOp>();
    if (!func) {
      return false;
    }
    auto &kernels = analysis.getKernelDialectInfo();
    auto kernelIt = kernels.find(func);
    if (kernelIt == kernels.end()) {
      return false;
    }
    auto &views = kernelIt->second.OpQViewMap;
    auto viewIt = views.find(op);
    if (viewIt == views.end()) {
      return false;
    }
    const QuantumOpView &view = viewIt->second; // const: don't create roles
    return !view.getQubits(QubitRole::Control).in.empty() ||
           !view.getQubits(QubitRole::Target).in.empty();
  }

  MyModuleAnalysis &analysis;
  bool recording = false;
  std::vector<Operation *> recordedGates;
};
} // namespace

namespace {

const std::unordered_map<Gate, std::vector<DecompositionRule>> &
getDecompositionTable(const QuantumDialect &DialectTy) {
  static const std::unordered_map<Gate, std::vector<DecompositionRule>> table =
      [] {
        std::unordered_map<Gate, std::vector<DecompositionRule>> t;
        t[Gate::H] = {
            // Always emits a fixed pi/2 Rx, i.e. "sx" (sqrt-X), not a
            // generic parameterized one -- see the comment on
            // rewriteHToRzRxRz.
            makeRule("HToRzSxRz", {Gate::RZ, Gate::SX}, rewriteHToRzRxRz),
            makeRule("HToU3", {Gate::U3}, rewriteHToU3),
            makeRule("HToPhasedRx", {Gate::PhasedRx}, rewriteHToPhasedRx)};
        t[Gate::PauliX] = {
            makeRule("XToHZH", {Gate::H, Gate::PauliZ}, rewriteXToHZH),
            makeRule("XToRx", {Gate::RX}, rewriteXToRx),
            makeRule("XToPhasedRx", {Gate::PhasedRx}, rewriteXToPhasedRx)};
        t[Gate::PauliY] = {
            makeRule("YToRy", {Gate::RY}, rewriteYToRy),
            makeRule("YToPhasedRx", {Gate::PhasedRx}, rewriteYToPhasedRx)};
        t[Gate::PauliZ] = {
            makeRule("ZToHXH", {Gate::H, Gate::PauliX}, rewriteZToHXH),
            makeRule("ZToRz", {Gate::RZ}, rewriteZToRz)};
        t[Gate::S] = {
            makeRule("SToRz", {Gate::RZ}, rewriteSToRz),
            makeRule("SToSdgSdgSdg", {Gate::SAdj}, rewriteSToSdgSdgSdg),
            makeRule("SToTT", {Gate::T}, rewriteSToTT),
            makeRule("SToPhasedRx", {Gate::PhasedRx}, rewriteSToPhasedRx)};
        t[Gate::SAdj] = {makeRule("SdgToRz", {Gate::RZ}, rewriteSdgToRz),
                         makeRule("SdgToSSS", {Gate::S}, rewriteSdgToSSS)};
        t[Gate::T] = {
            makeRule("TToRz", {Gate::RZ}, rewriteTToRz),
            makeRule("TToPhasedRx", {Gate::PhasedRx}, rewriteTToPhasedRx)};
        t[Gate::TAdj] = {makeRule("TdgToRz", {Gate::RZ}, rewriteTdgToRz)};
        t[Gate::R1] = {
            makeRule("R1ToRz", {Gate::RZ}, rewriteR1ToRz),
            makeRule("R1ToPhasedRx", {Gate::PhasedRx}, rewriteR1ToPhasedRx)};
        t[Gate::RX] = {
            makeRule("RxToHRzH", {Gate::H, Gate::RZ}, rewriteRxToHRzH),
            makeRule("RxToPhasedRx", {Gate::PhasedRx}, rewriteRxToPhasedRx)};
        // "sx" is the fixed pi/2 special case of "rx" (see CheckforSXOp); reuse
        // the same prx fallback rather than duplicating it. It has no
        // fallback through "h"/"rz" because that path is exactly how "h"
        // itself produces "sx" in the first place (HToRzSxRz above) -- a
        // rule the other way would just bounce the two back and forth.
        t[Gate::SX] = {
            makeRule("SxToPhasedRx", {Gate::PhasedRx}, rewriteRxToPhasedRx)};
        t[Gate::RY] = {
            makeRule("RyToRzRxRz", {Gate::RZ, Gate::RX}, rewriteRyToRzRxRz)};
        t[Gate::RZ] = {
            makeRule("RzToHRxH", {Gate::H, Gate::RX}, rewriteRzToHRxH),
            makeRule("RzToU3", {Gate::U3}, rewriteRzToU3),
            makeRule("RzToPhasedRx", {Gate::PhasedRx}, rewriteRzToPhasedRx)};
        t[Gate::U2] = {
            makeRule("U2ToRzRyRz", {Gate::RZ, Gate::RY}, rewriteU2ToRzRyRz),
            makeRule("U2ToU3", {Gate::U3}, rewriteU2ToU3)};
        t[Gate::U3] = {
            makeRule("U3ToRzRyRz", {Gate::RZ, Gate::RY}, rewriteU3ToRzRyRz)};
        t[Gate::SWAP] = {makeRule("SwapToUpperCxCxCx", {Gate::CNOT},
                                  rewriteSwapToUpperCxCxCx)};
        t[Gate::CNOT] = {makeRule("CxToUpperHCzH", {Gate::H, Gate::CZ},
                                  rewriteCxToUpperHCzH)};
        t[Gate::CZ] = {makeRule("CzToUpperHCxH", {Gate::H, Gate::CNOT},
                                rewriteCzToUpperHCxH)};
        t[Gate::CY] = {makeRule("CyToSCxSdg", {Gate::S, Gate::CNOT, Gate::SAdj},
                                rewriteCyToSCxSdg)};
        t[Gate::CRX] = {
            makeRule("CrxToHCrzH", {Gate::H, Gate::CRZ}, rewriteCrxToHCrzH)};
        t[Gate::CRZ] = {
            makeRule("CrzToHCrxH", {Gate::H, Gate::CRX}, rewriteCrzToHCrxH),
            makeRule("CrzToRzCu3", {Gate::RZ, Gate::U3}, rewriteCrzToRzCu3),
            makeRule("CrzToRzCxRzCx", {Gate::RZ, Gate::CNOT},
                     rewriteCrzToRzCxRzCx)};
        t[Gate::CRY] = {makeRule("CryToRzCrxRz", {Gate::RZ, Gate::CRX},
                                 rewriteCryToRzCrxRz)};
        return t;
      }();
  return table;
}

//===----------------------------------------------------------------------===//
// External gate names. The 'gates=' pass option and QDMI devices name gates
// with lowercase mnemonics ("cx", "sdg", "prx", ...); internally everything is
// keyed by the Gate enum. This table is the single place that translates
// between the two (after resolveMnemonicAlias has mapped device-specific
// aliases such as "r" onto these names).
//===----------------------------------------------------------------------===//

using NativeGateSet = std::unordered_set<Gate>;

struct MnemonicEntry {
  llvm::StringLiteral name;
  Gate gate;
};

// TODO: Can we get rid of this and instead move the mnemonics
// to parseGateTy(const StringRef &GateTy) in Extractor.h?
const MnemonicEntry (&getMnemonicTable())[23] {
  static const MnemonicEntry table[] = {
      {"h", Gate::H},          {"x", Gate::PauliX},  {"y", Gate::PauliY},
      {"z", Gate::PauliZ},     {"s", Gate::S},       {"sdg", Gate::SAdj},
      {"t", Gate::T},          {"tdg", Gate::TAdj},  {"sx", Gate::SX},
      {"r1", Gate::R1},        {"rx", Gate::RX},     {"ry", Gate::RY},
      {"rz", Gate::RZ},        {"u2", Gate::U2},     {"u3", Gate::U3},
      {"prx", Gate::PhasedRx}, {"swap", Gate::SWAP}, {"cx", Gate::CNOT},
      {"cz", Gate::CZ},        {"cy", Gate::CY},     {"crx", Gate::CRX},
      {"cry", Gate::CRY},      {"crz", Gate::CRZ},
  };
  return table;
}

// External name -> Gate. Names this pass has no Gate for (e.g. "measure",
// "reset", or a typo) yield std::nullopt: they can never be a native gate here.
static std::optional<Gate> parseMnemonic(llvm::StringRef name) {
  for (const auto &entry : getMnemonicTable())
    if (entry.name == name)
      return entry.gate;
  return std::nullopt;
}

// Gate -> external name, for diagnostics.
static llvm::StringRef mnemonicName(Gate gate) {
  for (const auto &entry : getMnemonicTable())
    if (entry.gate == gate)
      return entry.name;
  return "<unknown>";
}

static bool CheckforSXOp(const mlir::Value &param) {
  // A fixed pi/2 rotation about X is "sx" (sqrt-X): its own native gate on
  // hardware whose basis doesn't include an arbitrary-angle Rx (e.g. WMI's
  // {cz, x, y, rz, sx}). Distinguish it from a genuinely parameterized
  // "rx" so it isn't left unresolved on such targets, and doesn't get
  // bounced back and forth with the "h" rule that produces it.
  // (Rx always carries exactly one parameter.)
  if (auto cst = param.getDefiningOp<mlir::arith::ConstantOp>()) {
    if (auto fp = dyn_cast<FloatAttr>(cst.getValue())) {
      if (std::abs(fp.getValueAsDouble() - M_PI_2) < 1e-9)
        return true;
    }
  }

  return false;
}

// "sx" (a fixed pi/2 rotation about X) is a strictly weaker ask than generic
// "rx": any target that already declares arbitrary-angle "rx" native can
// trivially perform its pi/2 special case too. Fold that one-way implication
// into a single legality check used everywhere this pass asks "is this
// mnemonic native?", instead of duplicating it at each call site.
bool isNative(const NativeGateSet &native, Gate gate) {
  return native.count(gate) != 0 ||
         (gate == Gate::SX && native.count(Gate::RX) != 0);
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
std::unordered_map<Gate, const DecompositionRule *> computeWitnesses(
    const std::unordered_map<Gate, std::vector<DecompositionRule>> &table,
    const NativeGateSet &native) {
  std::unordered_map<Gate, const DecompositionRule *> witness;

  while (true) {
    std::unordered_map<Gate, const DecompositionRule *> picked_rules;
    for (const auto &[table_gate, rules_list] : table) {
      if (isNative(native, table_gate) || witness.count(table_gate)) {
        continue;
      }
      for (auto &rule : rules_list) {
        bool allProducedNative = true;
        for (const auto &produced_gate : rule.produces) {
          if (!isNative(native, produced_gate) &&
              !witness.count(produced_gate)) {
            allProducedNative = false;
            break;
          }
        }
        if (allProducedNative) {
          picked_rules[table_gate] = &rule;
          break;
        }
      }
    }

    if (picked_rules.empty())
      break;

    for (auto &[gate, rule] : picked_rules) {
      witness[gate] = rule;
    }
  }

  for (auto &[gate, rule] : witness) {
    MQSS_DEBUG("For gate: " << parseGateTy(gate) << " : ");
    for (const auto &produced_gate : rule->produces) {
      MQSS_DEBUG(parseGateTy(produced_gate) << " ");
    }
    MQSS_DEBUG("\n----------\n");
  }

  return witness;
}

// Expected qubit count and parameter count for each gate.
// Used to sanity check a QDMI reported operation before trusting it.
// For example a device reporting rz with 0 parameters is not our rz.
const std::unordered_map<Gate, std::pair<size_t, size_t>> &getGateArity() {
  static const std::unordered_map<Gate, std::pair<size_t, size_t>> arity = {
      {Gate::H, {1, 0}},        {Gate::PauliX, {1, 0}}, {Gate::PauliY, {1, 0}},
      {Gate::PauliZ, {1, 0}},   {Gate::S, {1, 0}},      {Gate::SAdj, {1, 0}},
      {Gate::T, {1, 0}},        {Gate::TAdj, {1, 0}},   {Gate::SX, {1, 0}},
      {Gate::R1, {1, 1}},       {Gate::RX, {1, 1}},     {Gate::RY, {1, 1}},
      {Gate::RZ, {1, 1}},       {Gate::U2, {1, 2}},     {Gate::U3, {1, 3}},
      {Gate::PhasedRx, {1, 2}}, {Gate::SWAP, {2, 0}},   {Gate::CNOT, {2, 0}},
      {Gate::CZ, {2, 0}},       {Gate::CY, {2, 0}},     {Gate::CRX, {2, 1}},
      {Gate::CRY, {2, 1}},      {Gate::CRZ, {2, 1}},
  };
  return arity;
}

// Builds the native gate set from a QDMI device.
// Operation names go through resolveMnemonicAlias and then parseMnemonic.
// Operations this pass has no Gate for (measure, reset, ...) are skipped, and
// an operation with the wrong arity for its name is dropped with a warning.
llvm::Expected<NativeGateSet>
getNativeGateSetFromQDMI(const std::string &conf, const std::string &deviceName,
                         Location loc) {
  auto device = createQDMIDevice(
      conf.c_str(), deviceName.empty() ? nullptr : deviceName.c_str());
  if (!device)
    return device.takeError();

  auto gateSet = getDeviceNativeGateSet(*device);
  if (!gateSet)
    return gateSet.takeError();

  const auto &arity = getGateArity();
  NativeGateSet native;
  for (const GateOperationInfo &gate : *gateSet) {
    llvm::StringRef mnemonic = resolveMnemonicAlias(gate.gate_name);
    std::optional<Gate> gateTy = parseMnemonic(mnemonic);
    if (!gateTy)
      continue;
    auto it = arity.find(*gateTy);
    if (it != arity.end() && gate.num_qubits && gate.num_parameters &&
        (*gate.num_qubits != it->second.first ||
         *gate.num_parameters != it->second.second)) {
      mlir::emitWarning(loc)
          << "BasisConversion: QDMI device operation '" << gate.gate_name
          << "' (" << *gate.num_qubits << " qubit(s), " << *gate.num_parameters
          << " parameter(s)) does not match this pass's expected shape "
             "for '"
          << mnemonic << "', ignoring it as a native gate.";
      continue;
    }
    native.insert(*gateTy);
  }

  if (native.empty())
    return llvm::createStringError(
        llvm::inconvertibleErrorCode(),
        "QDMI device reported no operations usable as a native gate set");
  return native;
}

struct BasisConversion
    : public mqss::mqssci::codegen::impl::BasisConversionPassBase<
          BasisConversion> {
public:
  BasisConversion() = default;

  explicit BasisConversion(const BasisConversionPassOptions &options)
      : BasisConversion() {
    gates = options.gates;
    qdmi = options.qdmi;
    device = options.device;
  }

  void runOnOperation() override {
    bool wasApplied = false;

    MQSS_DEBUG("\n[Applying Pass: BasisConversion]\n");

    auto kernel = getOperation();
    NativeGateSet native;

    if (!qdmi.empty()) {
      if (gates.getNumOccurrences() > 0)
        mlir::emitWarning(kernel.getLoc())
            << "BasisConversion: both 'qdmi' and 'gates' were given; using "
               "the QDMI device's native gate set and ignoring 'gates'.";
      auto qdmiNative = getNativeGateSetFromQDMI(qdmi, device, kernel.getLoc());
      if (!qdmiNative) {
        mlir::emitError(kernel.getLoc())
            << "BasisConversion: " << llvm::toString(qdmiNative.takeError());
        return signalPassFailure();
      }
      native = std::move(*qdmiNative);
    } else {
      auto split_gates = split(gates, ",");
      for (const auto &gate : split_gates) {
        // Names this pass has no Gate for are ignored, as before.
        if (std::optional<Gate> gateTy =
                parseMnemonic(resolveMnemonicAlias(gate)))
          native.insert(*gateTy);
      }
    }

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    auto &kernel_info = analysis.getKernelDialectInfo();

    const auto &table = getDecompositionTable(DialectTy);
    // Computed once, up front: for every non-native mnemonic that can
    // transitively bottom out in native gates, the rule that gets it there.
    // Mnemonics absent from this map (but present in the table) are known
    // gates this pass genuinely cannot legalize into the requested set --
    // warned about once below, and left untouched, rather than repeatedly
    // rewritten with a rule that can never converge.
    const auto witnesses = computeWitnesses(table, native);
    std::unordered_set<Gate> warnedUnresolvable;

    // Applying a witness rule can itself introduce ops whose own witness
    // rule hasn't fired yet (e.g. crz -> {rz, cx}, then rz -> {h, rx}), so
    // this still needs multiple rounds; the loop is now just mechanically
    // unwinding an acyclic chain, so it always reaches a fixed point in at
    // most table.size() rounds. The cap is only a defensive backstop.
    constexpr int maxRounds = 64;
    AnalysisSyncListener listener(analysis);
    for (int round = 0; round < maxRounds; ++round) {
      bool changed = false;
      // Iterate only the module's top-level functions (no recursion into
      // their bodies), then walk each function's ops exactly once.
      for (auto func : kernel.getOps<mlir::func::FuncOp>()) {
        if (!kernel_info.count(func)) {
          continue;
        }

        auto &funcInfo = kernel_info[func];
        auto &funcQView = funcInfo.OpQViewMap;
        func->walk([&](Operation *op) {
          if (!funcQView.count(op)) {
            return;
          }
          // Deliberately a copy, not a reference into funcQView: applying a
          // rule inserts ops, and the listener adds them to this vector-backed
          // map, which would invalidate a reference into it.
          auto OpQView = funcQView[op];

          if (OpQView.isAdjoint && OpQView.isParameterized())
            return;

          auto gate = OpQView.GateTy;
          if (gate == Gate::UNKNOWN || gate == Gate::CH)
            return;

          if (gate == Gate::RX && CheckforSXOp(OpQView.params.back())) {
            gate = Gate::SX;
          }
          if (isNative(native, gate))
            return;
          auto wit = witnesses.find(gate);
          if (wit == witnesses.end()) {
            if (table.count(gate) && warnedUnresolvable.insert(gate).second)
              mlir::emitError(kernel.getLoc())
                  << "BasisConversion Op: '" << mnemonicName(gate)
                  << "' cannot be decomposed into the requested native gate "
                     "set"
                  << (table.count(gate)
                          ? ""
                          : " (no decomposition rule exists for it)");
            signalPassFailure();
            return;
          }
          IRRewriter rewriter(op->getContext(), &listener);

          // New gates go right after the op being decomposed. Set once here
          // so no individual rule has to (four of them used to forget, which
          // left their new ops un-inserted and silently dropped the gate).
          rewriter.setInsertionPointAfter(op);

          // Record only the gates this one rule application creates, in
          // creation order: the SSA fix-up below needs exactly that list.
          listener.startRecording();
          auto result = wit->second->apply(DialectTy, rewriter, op, OpQView);
          auto newGates = listener.stopRecording();
          if (!result) {
            mlir::emitError(func->getLoc(),
                            "BasisConversion: Failed to apply pass!");
            signalPassFailure();
            return;
          }
          if (listener.failed) {
            mlir::emitError(func->getLoc(),
                            "BasisConversion: Failed to add newly created Op!");
            signalPassFailure();
            return;
          }
          if (!replaceGateResolvingSSA(analysis, rewriter, op, OpQView,
                                       newGates)) {
            mlir::emitError(func->getLoc(),
                            "BasisConversion: Failed to restore SSA form!");
            signalPassFailure();
            return;
          }

          changed = true;
        });
      }
      if (!changed)
        break;

      // Some rules (e.g. CrzToRzCxRzCx) build a new op's angle out of an
      // arithmetic expression -- arith.negf/arith.divf on a prior operand --
      // rather than a literal constant. CheckforSXOp()'s "is this angle
      // exactly pi/2, i.e. sx?" check only matches a *direct*
      // arith::ConstantOp definer (see CheckforSXOp above), not a foldable
      // expression, so a rotation that's genuinely
      // pi/2 can be misclassified as generic "rx" purely because of how many
      // decomposition hops it went through -- which can leave it stuck
      // un-legalizable on a target whose native set has "sx" but not generic
      // "rx" (e.g. wmi's {cz, x, y, rz, sx}). Fold such expressions back down
      // to literal constants before the next round's CheckforSXOp() calls
      // re-inspect whatever this round just produced. An empty pattern set
      // is enough: GreedyRewriteConfig folds constant-foldable ops by
      // default, so this is purely a folding/DCE pass, not a general
      // canonicalization.
      mlir::RewritePatternSet emptyPatterns(kernel.getContext());
      mlir::GreedyRewriteConfig config; // for the folding call
      config.setListener(&listener);
      (void)mlir::applyPatternsGreedily(
          kernel, mlir::FrozenRewritePatternSet(std::move(emptyPatterns)),
          config);

      if (listener.failed) {
        mlir::emitError(kernel->getLoc(),
                        "BasisConversion: Failed to add newly created Op!");
        signalPassFailure();
        return;
      }

      if (round == maxRounds - 1)
        mlir::emitWarning(kernel.getLoc())
            << "BasisConversion: gave up after " << maxRounds
            << " rounds without reaching a fixed point";
    }
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
      signalPassFailure();
      return;
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::codegen::BasisConversionPass() {
  return std::make_unique<BasisConversion>();
}

std::unique_ptr<Pass> mqss::mqssci::codegen::createBasisConversionPass(
    const BasisConversionPassOptions &options) {
  return std::make_unique<BasisConversion>(options);
}
