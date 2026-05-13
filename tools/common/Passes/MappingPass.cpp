

#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

void performMapping(MyModuleAnalysis &analysis) {

  for (auto &[kernel, info] : analysis.getKernelDialectInfo()) {
    SmallSetVector<int, 4> InputQubits;
    SmallSetVector<int, 4> MeasureQubits;
    for (auto &[Op, QView] : info) {

      if (QView.isMeasureOp) {
        for (auto Qubit : QView.getQubits(QubitRole::Target).ids) {
          MeasureQubits.insert(Qubit.index);
        }
        continue;
      }

      for (auto Qubit : QView.getQubits(QubitRole::Control).ids) {
        InputQubits.insert(Qubit.index);
      }
      for (auto Qubit : QView.getQubits(QubitRole::Target).ids) {
        InputQubits.insert(Qubit.index);
      }
    }
    llvm::outs() << "\nkernel: " << kernel.getSymName()
                 << " total input qubits: " << InputQubits.size()
                 << " Measure qubits: " << MeasureQubits.size() << "\n\n";
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
