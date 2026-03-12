

// #include "Decompositions.hpp"
// #include "Examples.hpp"
// #include "Optimizer/Pipelines.hpp"
// #include "Passes/CodeGen.hpp"
// #include "Transforms.hpp"
// #include "Interfaces/Extractor.hpp"

#include "IR/QuantumDialect.h"
#include "MQSSCatalystPasses/Analysis.h"
#include "MQSSCatalystPasses/Transforms.h"
#include "mlir/IR/Dialect.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassRegistry.h>

using namespace llvm;
int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // ... your dialect setup ...

  // 2. Explicitly register the standard dialects
  // registerAllDialects(registry) often fails due to linker stripping symbols
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect>();

  // 3. Keep your custom dialects
  registry.insert<mlir::tensor::TensorDialect>();
  registry.insert<catalyst::quantum::QuantumDialect>();

  // For Catalyst / StableHLO (Maybe in the future)
  // mlir::DialectRegistry catRegistry;
  // catRegistry.insert<catalyst::quantum::QuantumDialect>();
  // mlir::MLIRContext catContext(catRegistry);

  registerMQSSTransformsPasses();

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mqss_catalyst::opt::createPrintCatalystGatesPass(llvm::outs());
  });

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCanonicalizerPass();
  });
  mlir::registerPass(
      []() -> std::unique_ptr<mlir::Pass> { return mlir::createCSEPass(); });

  llvm::outs() << "Dialects and Passes have been registered!\n";

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}