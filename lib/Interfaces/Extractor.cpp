

#include "Interfaces/Extractor.hpp"

#include "Support/CodeGen/Quake.hpp"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include <llvm/Support/raw_ostream.h>

using namespace mlir;
using namespace mqss;

class SemanticExtractor
    : public PassWrapper<SemanticExtractor, OperationPass<mlir::ModuleOp>> {

public:
  [[nodiscard]] llvm::StringRef getArgument() const override {
    return "extractor-pass";
  }
  [[nodiscard]] llvm::StringRef getDescription() const override {
    return "Extract features of an input dialect to run transformation passes "
           "on";
  }

  void runOnOperation() override {
    auto circuit = getOperation();
    llvm::outs() << "Printing Circuit functions:\n";
  
    // 2. Iterate over every function in the module
    for (auto func : circuit.getOps<mlir::func::FuncOp>()) {
      // Get the name as a StringRef (lightweight string view)
      llvm::StringRef name = func.getName();

      llvm::outs() << "Found function: " << name << "\n";

    }
  }
};

std::unique_ptr<mlir::Pass> mqss::opt::createExtractorPass() {
  return std::make_unique<SemanticExtractor>();
}