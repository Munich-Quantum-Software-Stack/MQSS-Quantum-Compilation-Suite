

#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;
using namespace qc;
namespace {

void loadControlledGates(mlir::Operation *gateOp, QuantumOpView qview,
                         QuantumComputation &qc) {

  auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((controlQubitVector.size() == 1 && targetQubitVector.size() == 1) &&
         "Only upto 2-Qubit gates supported!");
  auto controlQubit = controlQubitVector[0].index;
  auto targetQubit = targetQubitVector[0].index;

  if ((controlQubit == -1) && (targetQubit == -1)) {
    // Gate operation in Catalyst (value semantics)
    llvm::outs() << "Gate Op: " << *gateOp << "\n";
    // Qubit in Catalyst
    SmallVector<std::optional<QubitID>, 2> OriginQubits;
    for (auto operand : gateOp->getOperands()) {
      auto originOp = getOriginQubit(operand);
      OriginQubits.push_back(originOp);
    }
    if (qview.GateTy == Gate::CNOT) {
      qc.cx(OriginQubits[0]->index, OriginQubits[1]->index);
    }
  }
  // Gate operation in Quake (reference semantics)
}

void performMapping(MyModuleAnalysis &analysis) {

  for (auto &[kernel, info] : analysis.getKernelDialectInfo()) {
    
    llvm::outs() << "\nkernel: " << kernel.getSymName()
                 << " total input qubits: " << info.AllocatedQubits
                 << " Measure qubits: " << info.NumMeasureQubits << "\n\n";

    qc::QuantumComputation qc{info.AllocatedQubits, info.NumMeasureQubits};
    for (auto &[gateOp, qview] : info.OpQViewMap) {
      if (qview.GateTy != Gate::UNKNOWN && qview.isControlled()) {
        loadControlledGates(gateOp, qview, qc);
      }
    }

    llvm::errs() << "Dumping QC:\n";
    qc.print(std::cout);
  }
}

class Mapping : public mqss_backend::CommonMappingPassBase<Mapping> {

  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.

    llvm::outs() << "\n[Applying Pass: MappingPass]\n";

    auto &analysis = getAnalysis<DialectAnalysis>();
    auto KernelDialectInfo = analysis.getKernelDialectInfo();

    performMapping(analysis);
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonMappingPass() {
  return std::make_unique<Mapping>();
}
