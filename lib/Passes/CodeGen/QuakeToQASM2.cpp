#include "cudaq/Optimizer/CodeGen/OpenQASMEmitter.h"
#include "Passes/transforms/PassIncludes.h"
#include "Passes/transforms/Transforms.h"
#include "Passes/transforms/Error.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

#include "llvm/IR/Module.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"

namespace mqss::opt {

#define GEN_PASS_DEF_QUAKETOQASM2PASS
#include "Passes/transforms/Transforms.h.inc"

} // namespace mqss::opt

using namespace mlir;
using namespace llvm;

namespace {

class QuakeToQASM2
    : public mqss_backend::impl::QuakeToQASM2PassBase<QuakeToQASM2> {

public:

  void runOnOperation() override {

    MQSS_DEBUG("\n[Applying Pass: QuakeToQASM2]\n");
    auto context = &getContext();
    mlir::ModuleOp module = getOperation();

    MQSS_DEBUG("\nTranslated output:\n");

    translateToOpenQASM(module, llvm::outs());
    // Convert the module to LLVM IR in a new LLVM IR context.
    
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::QuakeToQASM2Pass() {
  return std::make_unique<QuakeToQASM2>();
}
