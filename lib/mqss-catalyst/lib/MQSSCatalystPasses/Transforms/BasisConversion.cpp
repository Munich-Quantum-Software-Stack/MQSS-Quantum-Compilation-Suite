

#include "IR/QuantumOps.h"
#include "MQSSCatalystPasses/Transforms.h"
#include "Utils/quantumutils.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/raw_ostream.h>

namespace mqss_catalyst::opt {
#define GEN_PASS_CLASSES
#include "MQSSCatalystPasses/Transforms.h.inc"

} // namespace mqss_catalyst::opt

using namespace mlir;
using namespace llvm;
using namespace catalyst::quantum;

struct NativeGateSet {
  llvm::StringSet<> allowedGates;

  [[nodiscard]] bool isAllowed(llvm::StringRef name) const {
    return allowedGates.contains(name);
  }
};

NativeGateSet getIBMBasis() {
  NativeGateSet basis;
  basis.allowedGates.insert("RZ");
  basis.allowedGates.insert("SX");
  basis.allowedGates.insert("CZ");
  return basis;
}

NativeGateSet getQuantinumBasis() {
  NativeGateSet basis;
  basis.allowedGates.insert("RZ");
  basis.allowedGates.insert("SX");
  basis.allowedGates.insert("CZ");
  return basis;
}

// ── Pattern: Hadamard → RZ(π/2) SX RZ(π/2) ──────────────────────────────
struct HadamardToRZSXCZPattern
    : public OpRewritePattern<catalyst::quantum::CustomOp> {
  using OpRewritePattern<catalyst::quantum::CustomOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(catalyst::quantum::CustomOp op,
                                PatternRewriter &rewriter) const override {
    // Only match "Hadamard"

    if ((op.getGateName() != "Hadamard") && (op.getGateName() != "H"))
      return failure();

    Location loc = op.getLoc();
    Value qin = op.getInQubits()[0];

    // Build constants
    auto pi_2 = rewriter.create<arith::ConstantFloatOp>(
        loc, rewriter.getF64Type(), llvm::APFloat(M_PI_2));

    // RZ(π/2) → SX → RZ(π/2)
    auto rz1 = rewriter.create<quantum::CustomOp>(
        loc, mlir::TypeRange{qin.getType()}, mlir::ValueRange{},
        mlir::ValueRange{pi_2}, mlir::ValueRange{qin},
        rewriter.getStringAttr("RZ"),
        /*adjoint=*/false, mlir::ValueRange{}, mlir::ValueRange{});

    MQSS_DEBUG("RZ1: " << *rz1 << " : " << rz1.getOutQubits().size() << "\n");

    auto sx = rewriter.create<quantum::CustomOp>(
        loc, mlir::TypeRange{qin.getType()}, mlir::ValueRange{},
        mlir::ValueRange{}, mlir::ValueRange{rz1.getOutQubits()[0]},
        rewriter.getStringAttr("SX"),
        /*adjoint=*/false, mlir::ValueRange{}, mlir::ValueRange{});

    MQSS_DEBUG("SX: " << *sx << " : " << sx.getOutQubits().size() << "\n");

    auto rz2 = rewriter.create<quantum::CustomOp>(
        loc, mlir::TypeRange{qin.getType()}, mlir::ValueRange{},
        mlir::ValueRange{pi_2}, mlir::ValueRange{sx.getOutQubits()[0]},
        rewriter.getStringAttr("RZ"),
        /*adjoint=*/false, mlir::ValueRange{}, mlir::ValueRange{});

    MQSS_DEBUG("RZ2: " << *rz2 << "\n");

    rewriter.replaceOp(op, rz2.getOutQubits());
    return success();
  }
};

// ── Pattern: Hadamard → RZ(π/2) SX RZ(π/2) ──────────────────────────────
struct HadamardToPhasedRXPattern
    : public OpRewritePattern<catalyst::quantum::CustomOp> {
  using OpRewritePattern<catalyst::quantum::CustomOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(catalyst::quantum::CustomOp op,
                                PatternRewriter &rewriter) const override {
    // Only match "Hadamard"

    if ((op.getGateName() != "Hadamard") && (op.getGateName() != "H"))
      return failure();

    Location loc = op.getLoc();
    Value qin = op.getInQubits()[0];

    auto pi = rewriter.create<arith::ConstantFloatOp>(
        loc, rewriter.getF64Type(), llvm::APFloat(M_PI));
    // Build constants
    auto pi_2 = rewriter.create<arith::ConstantFloatOp>(
        loc, rewriter.getF64Type(), llvm::APFloat(M_PI_2));

    // RZ(π/2) → SX → RZ(π/2)
    auto rz1 = rewriter.create<quantum::CustomOp>(
        loc, mlir::TypeRange{qin.getType()}, mlir::ValueRange{},
        mlir::ValueRange{pi_2}, mlir::ValueRange{qin},
        rewriter.getStringAttr("RZ"),
        /*adjoint=*/false, mlir::ValueRange{}, mlir::ValueRange{});

    //rewriter.replaceOp(op, rz2.getOutQubits());
    return success();
  }
};


class BasisConversion
    : public mqss_catalyst::opt::BasisConversionPassBase<BasisConversion> {

  void runOnOperation() override {

    MQSS_DEBUG("[Applying Pass]: BasisConversion\n");
    auto func = getOperation();

    MLIRContext *ctx = func.getContext();

    RewritePatternSet patterns(ctx);

    // Register all your device-specific rewrite patterns
    if(device == "IBM")
      patterns.add<HadamardToRZSXCZPattern>(&getContext());
     else {
      getOperation()->emitError() << "invalid device: " << device;
      signalPassFailure();
    };

    // GreedyRewrite applies patterns to fixpoint —
    // this is what --decompose-lowering lacks
    if (failed(applyPatternsAndFoldGreedily(func, std::move(patterns))))
      return signalPassFailure();

    auto basis = getIBMBasis();
    func.walk([&](quantum::CustomOp op) {
      auto gatename = op.getGateName();

      llvm::outs() << "Checking: " << gatename << "\n";
      if (!gatename.empty() && !basis.isAllowed(gatename)) {
        op.emitError() << "gate not decomposed to native basis: "
                       << op.getGateName();
        signalPassFailure();
      }
    });

    // Important: verify that no illegal quantum.custom gates remain.
  }
};

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::BasisConversionPass() {
  return std::make_unique<BasisConversion>();
}