

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

// Take a snapshot of the Quantum Circuit before the Pass(es)
void mqss::mqssci::verify::VerifyPassInstrumentation::runBeforePass(
    Pass *pass, Operation *op) {
  // MQSS_DEBUG("-->[verify] runBeforePass: " << pass->getName() << "\n");

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

  MQSS_DEBUG("-->[verify] Pass: " << pass->getName() << "\n");

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

  bool verification_result = true;
  for (auto [func_name, vqc_ty] : cached_module_snapshot) {
    if (vqc_ty.qc1.empty() || vqc_ty.qc2.empty()) {
      MQSS_DEBUG("Skipping verification for: " << func_name
                                               << " as no snapshots exist\n");
      continue;
    }

    // checker_config is caller-supplied (see the constructor); it must not
    // be mutated here. EquivalenceCheckingManager's constructor already
    // performs the alternating-checker-unsupported fallback internally on
    // its own copy of the config, so there is nothing to do here beyond
    // constructing it -- mutating checker_config would only leak into the
    // *next* kernel/pass's check without affecting this one, since ecm has
    // already snapshotted the config by this point.
    ec::EquivalenceCheckingManager ecm(vqc_ty.qc1, vqc_ty.qc2, checker_config);
    MQSS_DEBUG("[verify] Result for " << func_name << " : ");
    ecm.run();
    switch (ecm.equivalence()) {
    case ec::EquivalenceCriterion::Equivalent:
      MQSS_DEBUG("Equivalent\n");
      break;
    case ec::EquivalenceCriterion::EquivalentUpToGlobalPhase:
      MQSS_DEBUG("Equivalent Upto global Phase\n");
      break;
    case ec::EquivalenceCriterion::EquivalentUpToPhase:
      MQSS_DEBUG("Equivalent Upto Phase\n");
      break;
    case ec::EquivalenceCriterion::ProbablyEquivalent:
      MQSS_DEBUG("Probably Equivalent\n");
      break;
    default:
      verification_result = false;
      MQSS_DEBUG("NOT equivalent\n");
      break;
    }
    llvm::outs() << "\n";
  }

  if (verification_result) {
    llvm::outs() << "[verify] Result for " << pass->getName() << " : Success\n";
  } else {
    // TODO: This is a failure signal that would bail an entire pipeline. Worth
    // revisiting.
    signalPassFailure(pass);
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
