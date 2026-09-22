

#include "Passes/Verification/Instrumentation.h"
#include "Utils/DebugUtils.h"
#include "ir/QuantumComputation.hpp"

using namespace mlir;
using namespace llvm;

// TODO: Currently equivalence check runs after every pass. The obvious fix,
// using PassInstrumentation's runBeforePipeline/runAfterPipeline is to snapshot
// once before the whole run and compare once after. Currently this does not
// work: those hooks are only invoked via Pass::runPipeline (mlir/Pass/Pass.h),
// i.e. when a pass dynamically schedules a nested OpPassManager on an
// operation. None of our passes do this, and mqss-opt's pipeline is flat (it
// matches the PassManager's own anchor type rather than being nested under it),
// so runBeforePipeline/ runAfterPipeline never fire here — confirmed
// empirically, not just theoretically.A true once-at-the-end mode would need to
// snapshot/compare from outside PassInstrumentation entirely — e.g. wrapping
// the pm.run(...) call in mqss-cc.cpp's own driver code rather than relying on
// pipeline-level instrumentation hooks.

// Create and Return a qc::QuantumComputation object (defined in MQT-Core).
// MQT-QCEC runs equivalence checks on this object.
qc::QuantumComputation
mqss::mqssci::verify::VerifyPassInstrumentation::createMQTQuantumComputation(
    std::size_t allocatedQubits, std::size_t numMeasureQubits,
    MapVector<Operation *, QuantumOpView> OpQView) {
  qc::QuantumComputation qc{allocatedQubits, numMeasureQubits};

  MapVector<mlir::Operation *, int> MeasureOps;
  SmallPtrSet<mlir::Operation *, 16> OpsToErase;
  SmallVector<SmallVector<mlir::Value, 2>> AllResults;

  for (auto &[Op, qview] : OpQView) {
    if (qview.GateTy != Gate::UNKNOWN) {
      loadGateOpsIntoQC(Op, qview, qc, qview.isControlled());
      OpsToErase.insert(Op);
    }

    if (qview.isMeasureOp) {
      loadMeasureOpIntoQC(qview, qc);
      MeasureOps[Op] = qview.measurements.size();
    }
  }

  return qc;
}

void performCheck(qc::QuantumComputation qc1, qc::QuantumComputation qc2,
                  ec::Configuration config) {

  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);

  // If the AlternatingChecker structurally cannot handle the pair
  // of circuits, fallback to ConstructionChecker
  if (config.execution.runAlternatingChecker &&
      !ec::DDAlternatingChecker::canHandle(qc1, qc2)) {
    config.execution.runAlternatingChecker = false;
    config.execution.runConstructionChecker = true;
  }

  ecm.run();
  switch (ecm.equivalence()) {
  case ec::EquivalenceCriterion::Equivalent:
    llvm::outs() << "Equivalent\n";
    break;
  case ec::EquivalenceCriterion::EquivalentUpToGlobalPhase:
    llvm::outs() << "Equivalent Upto global Phase\n";
    break;
  case ec::EquivalenceCriterion::EquivalentUpToPhase:
    llvm::outs() << "Equivalent Upto Phase\n";
    break;
  case ec::EquivalenceCriterion::ProbablyEquivalent:
    llvm::outs() << "Probably Equivalent\n";
    break;
  default:
    llvm::outs() << "NOT equivalent\n";
    break;
  }
  llvm::outs() << "\n";
}

// Take a snapshot of the Quantum Circuit before the Pass(es)
void mqss::mqssci::verify::VerifyPassInstrumentation::runBeforePass(
    Pass *pass, Operation *op) {
  MQSS_DEBUG("-->[verify] runBeforePass: " << pass->getName() << "\n");

  DialectAnalysisSelector selector(op);
  auto &analysis = *selector.get();
  for (auto [kernel, info] : analysis.getKernelDialectInfo()) {

    MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                            << "total input qubits: " << info.AllocatedQubits
                            << " Measure qubits: " << info.NumMeasureQubits
                            << "\n");

    if (info.AllocatedQubits == 0)
      continue;

    auto name = kernel.getSymName();
    VerifyQuantumComputationTy vqc_ty;

    // vqc_ty.qc1 is the quantum circuit before the pass is invoked.
    vqc_ty.qc1 = createMQTQuantumComputation(
        info.AllocatedQubits, info.NumMeasureQubits, info.OpQViewMap);
    cached_module_snapshot[kernel.getSymName()] = std::move(vqc_ty);
  }
}

// After the pass(es), snapshot the circuit and compare this snapshot
// with the cached snapshot before the pass for equivalence
void mqss::mqssci::verify::VerifyPassInstrumentation::runAfterPass(
    Pass *pass, Operation *op) {

  MQSS_DEBUG("-->[verify] runAfterPass: " << pass->getName() << "\n");

  DialectAnalysisSelector selector(op);
  auto &analysis = *selector.get();
  for (auto [kernel, info] : analysis.getKernelDialectInfo()) {

    MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                            << "total input qubits: " << info.AllocatedQubits
                            << " Measure qubits: " << info.NumMeasureQubits
                            << "\n");

    if (info.AllocatedQubits == 0)
      continue;

    auto name = kernel.getSymName();
    assert(cached_module_snapshot.count(name) &&
           "instrumentation: function does not exist in cached module "
           "snapshot!");
    auto &vqc_ty = cached_module_snapshot[name];

    // vqc_ty.qc1 is the quantum circuit before the pass is invoked.
    vqc_ty.qc2 = createMQTQuantumComputation(
        info.AllocatedQubits, info.NumMeasureQubits, info.OpQViewMap);
  }

  // setup default configuration
  // Alternating, Simulation and ZX Checkers are all ON by default
  ec::Configuration config{};
  config = ec::Configuration{};
  config.functionality.checkPartialEquivalence = true;

  for (auto [func_name, vqc_ty] : cached_module_snapshot) {
    if (vqc_ty.qc1.empty() || vqc_ty.qc2.empty()) {
      MQSS_DEBUG("Skipping verification for: " << func_name
                                               << " as no snapshots exist\n");
      continue;
    }

    llvm::outs() << "[verify] " << func_name << ": ";
    performCheck(vqc_ty.qc1, vqc_ty.qc2, config);
  }
}

void mqss::mqssci::verify::VerifyPassInstrumentation::runAfterPassFailed(
    Pass *pass, Operation *op) {
  // runAfterPassFailed — the pass itself didn't complete; verification was
  // skipped
  llvm::errs() << "[verify] pass '" << pass->getName()
               << "' failed before equivalence could be checked (at "
               << op->getLoc() << ")\n";
}
