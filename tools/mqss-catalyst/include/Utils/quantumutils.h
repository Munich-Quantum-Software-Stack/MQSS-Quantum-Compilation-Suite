

#include "IR/QuantumOps.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;
using namespace catalyst;

inline quantum::CustomOp isCatalystQuantumGateOp(mlir::Operation *op) {

  if (auto g = llvm::dyn_cast<quantum::CustomOp>(op)) {
    return g;
  }
  return nullptr;
}

inline bool hasQuantumEffect(Operation *op) {
  for (auto type : op->getOperandTypes()) {
    if (isa<quantum::QubitType>(type))
      return true;
  }
  for (auto type : op->getResultTypes()) {
    if (isa<quantum::QubitType>(type))
      return true;
  }
  return false;
}

inline quantum::CustomOp
createCatalystGate(Location loc, llvm::StringRef NewGateTy,
                   const SmallVector<mlir::Value, 2> ControlQubitsOps,
                   const SmallVector<mlir::Value, 2> TargetQubitOps,
                   const mlir::ValueRange params, mlir::IRRewriter &builder,
                   bool isAdj = false) {

  quantum::CustomOp NewOp = nullptr;
  if (ControlQubitsOps.empty()) {
    std::vector<mlir::Type> TargetQubitTys;

    for (auto t : TargetQubitOps) {
      TargetQubitTys.push_back(t.getType());
    }
    if (NewGateTy == "SAdj") {
      NewGateTy = "S";
      isAdj = true;
    }

    return builder.create<quantum::CustomOp>(
        loc,
        /*out_qubits=*/mlir::TypeRange(TargetQubitTys),
        /*out_ctrl_qubits=*/mlir::TypeRange(),
        /*params=*/params,
        /*in_qubits=*/mlir::ValueRange(TargetQubitOps),
        /*gate_name=*/NewGateTy,
        /*adjoint=*/isAdj,
        /*in_ctrl_qubits=*/mlir::ValueRange(),
        /*in_ctrl_values=*/mlir::ValueRange());
    // TODO: Revisit this
    // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  }

  StringAttr GateTy;
  if (NewGateTy == "CNOT") {
    GateTy = builder.getStringAttr("CNOT");
  }
  if (NewGateTy == "CZ") {
    GateTy = builder.getStringAttr("CZ");
  }

  Value control = ControlQubitsOps[0];
  Value target = TargetQubitOps[0];

  return builder.create<quantum::CustomOp>(
      loc,
      /*out_qubits=*/mlir::TypeRange{control.getType(), target.getType()},
      /*out_ctrl_qubits=*/mlir::TypeRange{},
      /*params=*/mlir::ValueRange{},
      /*in_qubits=*/mlir::ValueRange{control, target},
      /*gate_name=*/GateTy, // or gateName if API
                            // accepts StringRef
      /*adjoint=*/isAdj,
      /*in_ctrl_qubits=*/mlir::ValueRange{},
      /*in_ctrl_values=*/mlir::ValueRange{});
  // TODO: Revisit this
  // OpToReplace->getResult(0).replaceAllUsesWith(NewOp->getResult(0));
  // OpToReplace->getResult(1).replaceAllUsesWith(NewOp->getResult(1));
}

inline mlir::arith::ConstantFloatOp
createCatalystConstOp(Location loc, mlir::IRRewriter &builder,
                      llvm::APFloat constantValue) {

  // Define the type as f64.
  auto floatType = builder.getF64Type();
  return builder.create<mlir::arith::ConstantFloatOp>(loc, floatType,
                                                      constantValue);
}

inline quantum::AllocOp createCatalystAlloca(Location loc,
                                             mlir::IRRewriter &builder,
                                             size_t numQubits) {

  auto regTy = quantum::ResultType::get(builder.getContext());
  mlir::Type qregTy = mlir::parseType("!quantum.reg", builder.getContext());
  assert(qregTy && "failed to parse !quantum.reg");

  return builder.create<quantum::AllocOp>(
      loc, qregTy,
      /*nqubits=*/mlir::Value{},
      /*nqubits_attr=*/builder.getI64IntegerAttr(numQubits));
}

inline quantum::ExtractOp createCatalystExtractRefOp(Location loc,
                                                     mlir::IRRewriter &builder,
                                                     Value allocQubits,
                                                     unsigned int targetQubit) {

  mlir::Type qbitTy = mlir::parseType("!quantum.bit", builder.getContext());

  assert(qbitTy && "failed to parse !quantum.bit");

  auto indexAttr = builder.getI64IntegerAttr(targetQubit);

  return builder.create<quantum::ExtractOp>(loc, qbitTy, allocQubits,
                                            /*index=*/mlir::Value{},
                                            /*index_attr=*/indexAttr);
}

inline SmallVector<Value, 2>
createCatalystMeasureOp(mlir::Location loc, mlir::IRRewriter &builder,
                        const SmallVector<mlir::Value, 2> TargetQubitOps) {

  SmallVector<Value> results;
  auto i1Ty = builder.getI1Type();
  for (Value q : TargetQubitOps) {
    auto qbitTy = q.getType(); // should be !quantum.bit
    auto m =
        builder.create<quantum::MeasureOp>(loc, TypeRange{i1Ty, qbitTy}, q);
    // measured classical result
    Value measResult = m.getMres();
    // updated SSA qubit
    Value outQubit = m.getOutQubit();

    results.push_back(measResult);
    results.push_back(outQubit);
  }
  // // Access results
  // mlir::Value outQubit = measureOp.getOutQubit(); // !quantum.bit
  // mlir::Value mres     = measureOp.getMres();     // i1
  return results;
}