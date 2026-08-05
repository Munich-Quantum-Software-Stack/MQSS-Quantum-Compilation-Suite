#pragma once

#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"

namespace mqss::opt {

// Registers every dialect and dialect-translation the MQSS passes operate on
// or lower into (Quake, Catalyst-quantum, standard MLIR dialects, LLVM
// translation). Call this before constructing an mlir::MLIRContext.
void registerMQSSDialects(mlir::DialectRegistry &registry);

// Convenience: builds a DialectRegistry via registerMQSSDialects() and
// returns a ready-to-use MLIRContext with all dialects loaded.
std::unique_ptr<mlir::MLIRContext> createMQSSContext();

} // namespace mqss::opt
