
#include "IR/Dialect/Quake/QuakeDialect.h"
#include "IR/Dialect/Quake/QuakeOps.h"
// #include "IR/Dialect/CC/CCOps.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;
inline std::tuple<bool, StringLiteral>

isQuakeQuantumGate(Operation *op) {

  if (auto x = dyn_cast<quake::XOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliX"};
    return {true, "CNOT"};
  }

  if (auto x = dyn_cast<quake::YOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliY"};
    return {true, "CY"};
  }
  if (auto x = dyn_cast<quake::ZOp>(op)) {
    if (x.getControls().size() == 0)
      return {true, "PauliZ"};
    return {true, "CZ"};
  }

  if (auto x = dyn_cast<quake::RxOp>(op))
    return {true, "RX"};
  if (auto x = dyn_cast<quake::RyOp>(op))
    return {true, "RY"};
  if (auto x = dyn_cast<quake::RzOp>(op))
    return {true, "RZ"};

  if (auto x = dyn_cast<quake::HOp>(op))
    return {true, "H"};

  if (auto x = dyn_cast<quake::SOp>(op)) {
    if (x.isAdj())
      return {true, "SAdj"};
    return {true, "S"};
  }
  if (auto t = dyn_cast<quake::TOp>(op))
    return {true, "T"};
  return {false, ""};
}

inline quake::OperatorInterface
createQuakeGate(Location loc, llvm::StringRef NewGateTy,
                const SmallVector<mlir::Value, 2> ControlQubits,
                const SmallVector<mlir::Value, 2> TargetQubitOps,
                const mlir::ValueRange params, mlir::IRRewriter &builder,
                bool isAdj = false) {

  if (NewGateTy == "RX") {
    // TODO: Can this create an gate?
    return builder.create<quake::RxOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RY") {
    // TODO: Can this create an gate?
    return builder.create<quake::RyOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "RZ") {
    // TODO: Can this create an gate?
    return builder.create<quake::RzOp>(loc, isAdj, params, ControlQubits,
                                       TargetQubitOps);
  }
  if (NewGateTy == "S") {
    return builder.create<quake::SOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "T") {
    return builder.create<quake::TOp>(loc, isAdj, params, ControlQubits,
                                      TargetQubitOps);
  }
  if (NewGateTy == "SAdj") {
    return builder.create<quake::SOp>(loc, true, params, ControlQubits,
                                      TargetQubitOps);
  }

  if (NewGateTy == "PauliZ") {
    return builder.create<quake::ZOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliX") {
    return builder.create<quake::XOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "PauliY") {
    return builder.create<quake::YOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "H") {
    return builder.create<quake::HOp>(loc, false, mlir::ValueRange(),
                                      mlir::ValueRange(), TargetQubitOps);
  }
  if (NewGateTy == "CNOT" || NewGateTy == "CX") {
    return builder.create<quake::XOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CY") {
    return builder.create<quake::YOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "CZ") {
    return builder.create<quake::ZOp>(loc, ControlQubits, TargetQubitOps);
  }
  if (NewGateTy == "SWAP") {
    return builder.create<quake::SwapOp>(loc, params, ControlQubits,
                                         TargetQubitOps);
  }
  return nullptr;
}

inline arith::ConstantOp createQuakeConstOp(Location loc,
                                            mlir::IRRewriter &builder,
                                            llvm::APFloat constantValue) {
  auto floatType = builder.getF64Type();
  auto NewOp = builder.create<mlir::arith::ConstantFloatOp>(loc, constantValue,
                                                            floatType);
  return NewOp;
}

inline quake::AllocaOp
createQuakeAlloca(Location loc, mlir::IRRewriter &builder, size_t numQubits) {

  return builder.create<quake::AllocaOp>(
      loc, quake::VeqType::get(builder.getContext(), numQubits));
}

inline quake::ExtractRefOp createQuakeExtractRefOp(Location loc,
                                                   mlir::IRRewriter &builder,
                                                   mlir::Value qubits,
                                                   unsigned int targetQubit) {

  return builder.create<quake::ExtractRefOp>(loc, qubits, targetQubit);
}

inline SmallVector<mlir::Value, 2>
createQuakeMeasureOp(Location loc, mlir::IRRewriter &builder,
                     const SmallVector<mlir::Value, 2> TargetQubits) {

  SmallVector<mlir::Value, 2> results;
  mlir::Type measTy = quake::MeasureType::get(builder.getContext());
  auto newOp = builder.create<quake::MzOp>(loc, measTy, TargetQubits);

  for (auto res : newOp->getResults()) {
    results.push_back(res);
  }

  return results;
}
