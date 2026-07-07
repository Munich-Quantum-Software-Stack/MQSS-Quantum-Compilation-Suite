
// #include "IR/CudaqQuake/CodeGen/Passes.h"
#include "Passes/transforms/PassIncludes.h"
#include "Passes/transforms/Transforms.h"
#include "Passes/transforms/Error.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

namespace mqss::opt {
#define GEN_PASS_DEF_LLVMDIALECTTOLLVMIRPASS
#include "Passes/transforms/Transforms.h.inc"
} // namespace mqss::opt

using namespace mlir;
using namespace llvm;

namespace {

class LLVMDialectToLLVMIR
    : public mqss_backend::impl::LLVMDialectToLLVMIRPassBase<LLVMDialectToLLVMIR> {

public:

  void runOnOperation() override {

    MQSS_DEBUG("\n[Applying Pass: LLVMDialectToLLVMIR]\n");
    auto context = &getContext();
    mlir::ModuleOp module = getOperation();

    MQSS_DEBUG("\nTranslated output:\n");

    // Convert the module to LLVM IR in a new LLVM IR context.
    llvm::LLVMContext llvmContext;
    auto llvmModule = translateModuleToLLVMIR(module, llvmContext);

    if (!llvmModule)
      mqss::opt::MQSSemitFatalError(module->getLoc(), "Failed to emit LLVM IR");

    llvmModule->dump();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::LLVMDialectToLLVMIRPass() {
  return std::make_unique<LLVMDialectToLLVMIR>();
}
