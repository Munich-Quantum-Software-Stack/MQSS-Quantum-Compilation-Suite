

#include "Passes/Transforms/TranspilationPassUtils.h"
#include "sc/exact/ExactMapper.hpp"
#include "sc/heuristic/HeuristicMapper.hpp"
// #include "ir/QuantumComputation.hpp"

using namespace mlir;

std::optional<double> getConstantDouble(mlir::Value v) {
  auto defOp = v.getDefiningOp<mlir::arith::ConstantOp>();
  if (!defOp)
    return std::nullopt;

  auto attr = llvm::dyn_cast<mlir::FloatAttr>(defOp.getValue());
  if (!attr)
    return std::nullopt;

  return attr.getValueAsDouble();
}

void resolveSSAformForMeasureOps(mlir::Operation *OldMeasOp,
                                 SmallVector<mlir::Value, 2> newResults) {
  // llvm::outs() << "Old meas op: " << *OldMeasOp
  //              << " results: " << OldMeasOp->getResults().size() << ","
  //              << newResults.size() << "\n";
  auto OldResults = OldMeasOp->getResults();
  for (int j = 0; j < OldResults.size(); ++j) {
    auto oldRes = OldResults[j];
    auto newRes = newResults[j];
    oldRes.replaceAllUsesWith(newRes);
  }
  assert(OldMeasOp->use_empty() &&
         "Old measurement Op should not have any users before erasing!");
  OldMeasOp->erase();
}

// Performs a controlled Dead-Code elimination optimization.
// It is less aggressive than MLIR's canonicalize and cse optimizations.
// canonicalize and cse can remove newly introduced gates.
void controlledDCE(SmallPtrSet<mlir::Operation *, 16> OpsToErase,
                   mlir::Value OldAllocaOp, mlir::Value newAllocOp) {
  llvm::SmallPtrSet<mlir::Operation *, 16> operandsToCleanup;

  for (mlir::Operation *op : OpsToErase) {

    for (mlir::Value operand : op->getOperands()) {
      if (!OpsToErase.contains(operand.getDefiningOp()))
        operandsToCleanup.insert(operand.getDefiningOp());
    }
  }

  eraseOpsSafely(OpsToErase);
  eraseOpsSafely(operandsToCleanup);

  OldAllocaOp.replaceAllUsesWith(newAllocOp);
  assert(OldAllocaOp.use_empty() &&
         "Old alloca cannot have uses before being erased!");
  OldAllocaOp.getDefiningOp()->erase();
}

// Load the measurement operation within QuantumComputation
static void loadMeasureOpIntQC(QuantumOpView qview,
                               qc::QuantumComputation &qc) {

  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((targetQubitVector.size() == 1) &&
         "Only Single Qubit Measurement Ops supported");

  auto targetQubit = targetQubitVector[0].index;
  for (auto entry : qview.measurements) {
    qc.measure(entry.QubitIndex, entry.ClassicalBitIndex);
  }
}

static void loadGates(mlir::Operation *gateOp, qc::QuantumComputation &qc,
                      int64_t controlQubit, int64_t targetQubit, Gate GateTy,
                      SmallVector<mlir::Value, 2> params) {

  // llvm::outs() << "Gate Op: " << *gateOp << " , Ty: " << GateTy << "\n";

  std::optional<double> angle;
  // TODO: Load more gates into qc. Add to the following list.
  switch (GateTy) {
  case Gate::CNOT:
    qc.cx(controlQubit, targetQubit);
    break;
  case Gate::CY:
    qc.cy(controlQubit, targetQubit);
    break;
  case Gate::CZ:
    qc.cz(controlQubit, targetQubit);
    break;
  case Gate::PauliX:
    qc.x(targetQubit);
    break;
  case Gate::PauliY:
    qc.y(targetQubit);
    break;
  case Gate::PauliZ:
    qc.z(targetQubit);
    break;
  case Gate::S:
    qc.s(targetQubit);
    break;
  case Gate::T:
    qc.t(targetQubit);
    break;
  case Gate::RX:
    assert(params.size() == 1 && "RX gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rx(angle.value(), targetQubit);
    break;
  case Gate::RY:
    assert(params.size() == 1 && "RY gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.ry(angle.value(), targetQubit);
    break;
  case Gate::RZ:
    assert(params.size() == 1 && "RZ gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rz(angle.value(), targetQubit);
    break;
  }
}

void loadGateOpsIntoQC(mlir::Operation *gateOp, QuantumOpView qview,
                       qc::QuantumComputation &qc, bool isControlled = false) {

  int64_t controlQubitIdx = -2;
  int64_t targetQubitIdx = -2;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;

  if (isControlled) {
    auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
    assert((controlQubitVector.size() == 1) &&
           (targetQubitVector.size() == 1) &&
           "Only upto 2-Qubit gates supported!");

    controlQubitIdx = controlQubitVector[0].index;
    if (controlQubitIdx == -1) {
      // Gate operation in Catalyst (value semantics)
      // Qubit in Catalyst
      auto operand = controlQubitVector[0].base;
      controlQubitIdx = getOriginQubit(operand)->index;
    }
  }
  assert((targetQubitVector.size() == 1) &&
         "Only upto 1-Qubit Non-controlled gates supported!");

  targetQubitIdx = targetQubitVector[0].index;

  if (targetQubitIdx == -1) {
    // Gate operation in Catalyst (value semantics)
    // Qubit in Catalyst
    auto operand = targetQubitVector[0].base;
    targetQubitIdx = getOriginQubit(operand)->index;
  }
  loadGates(gateOp, qc, controlQubitIdx, targetQubitIdx, qview.GateTy,
            qview.params);
}
