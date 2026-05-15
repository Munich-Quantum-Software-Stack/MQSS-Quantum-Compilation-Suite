

#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;

namespace {

void performMapping(MyModuleAnalysis &analysis) {

  for (auto &[kernel, info] : analysis.getKernelDialectInfo()) {

    llvm::outs() << "\nkernel: " << kernel.getSymName()
                 << " total input qubits: " << info.AllocatedQubits
                 << " Measure qubits: " << info.NumMeasureQubits << "\n\n";
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
