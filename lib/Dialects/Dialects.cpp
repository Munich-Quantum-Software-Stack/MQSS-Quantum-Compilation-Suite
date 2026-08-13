#include "Passes/Transforms/Dialects.h"

#include "Quantum/IR/QuantumDialect.h"
#include "cudaq/Optimizer/Dialect/CC/CCDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"

#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/ControlFlow/IR/ControlFlow.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>
#include <mlir/Dialect/Linalg/IR/Linalg.h>
#include <mlir/Dialect/SCF/IR/SCF.h>
#include <mlir/Dialect/Tensor/IR/Tensor.h>
#include <mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h>
#include <mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h>

namespace mqss::opt {

void registerMQSSDialects(mlir::DialectRegistry &registry) {
  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::linalg::LinalgDialect,
                  mlir::scf::SCFDialect, mlir::LLVM::LLVMDialect>();
  registry.insert<mlir::tensor::TensorDialect>();
  registry.insert<cudaq::cc::CCDialect, cudaq::quake::QuakeDialect>();
  registry.insert<catalyst::quantum::QuantumDialect>();

  mlir::registerBuiltinDialectTranslation(registry);
  mlir::registerLLVMDialectTranslation(registry);
}

std::unique_ptr<mlir::MLIRContext> createMQSSContext() {
  mlir::DialectRegistry registry;
  registerMQSSDialects(registry);
  auto context = std::make_unique<mlir::MLIRContext>(registry);
  context->loadAllAvailableDialects();
  return context; // unique_ptr is movable; the MLIRContext itself never moves
}

} // namespace mqss::opt
