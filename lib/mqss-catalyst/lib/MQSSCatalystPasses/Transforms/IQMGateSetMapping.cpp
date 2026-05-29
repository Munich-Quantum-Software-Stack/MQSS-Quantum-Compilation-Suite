

#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "IR/QuantumOps.h"
#include "Utils/quantumutils.h"
#include "MQSSCatalystPasses/Transforms.h"

namespace mqss_catalyst::opt {
#define GEN_PASS_CLASSES
#include "MQSSCatalystPasses/Transforms.h.inc"

} // namespace mqss_catalyst::opt

using namespace mlir;
using namespace catalyst::quantum;

// ── Pattern: Hadamard → RZ(π/2) SX RZ(π/2) ──────────────────────────────
struct HadamardToIQMPattern : OpRewritePattern<CustomOp> {
    using OpRewritePattern::OpRewritePattern;

    LogicalResult matchAndRewrite(CustomOp op,
                                  PatternRewriter &rewriter) const override {
        // Only match "Hadamard"
        if (op.getGateName() != "Hadamard")
            return failure();

        Location loc = op.getLoc();
        Value qin = op.getInQubits()[0];

        // Build constants
        auto pi_2 = rewriter.create<arith::ConstantFloatOp>(
            loc, rewriter.getF64Type(), llvm::APFloat(M_PI_2));

        // RZ(π/2) → SX → RZ(π/2)
        auto rz1 = rewriter.create<CustomOp>(
            loc, mlir::TypeRange{qin.getType()}, mlir::TypeRange{},
            mlir::ValueRange{pi_2}, mlir::ValueRange{qin},
            rewriter.getStringAttr("RZ"), /*adjoint=*/false, mlir::ValueRange{},
            mlir::ValueRange{});

        auto sx = rewriter.create<CustomOp>(
            loc, mlir::TypeRange{qin.getType()}, ValueRange{},
            ValueRange{rz1.getOutQubits()[0]},
            rewriter.getStringAttr("SX"), /*adjoint=*/false, mlir::ValueRange{},
            mlir::ValueRange{});

        auto rz2 = rewriter.create<CustomOp>(
            loc, mlir::TypeRange{qin.getType()}, ValueRange{pi_2},
            ValueRange{sx.getOutQubits()[0]},
            rewriter.getStringAttr("RZ"), /*adjoint=*/false, mlir::ValueRange{},
            mlir::ValueRange{});

        rewriter.replaceOp(op, rz2.getOutQubits());
        return success();
    }
};

// ── Pattern: CNOT → CZ with basis changes ────────────────────────────────
struct CNOTToIQMPattern : OpRewritePattern<CustomOp> {
    using OpRewritePattern::OpRewritePattern;

    LogicalResult matchAndRewrite(CustomOp op,
                                  PatternRewriter &rewriter) const override {
        if (op.getGateName() != "CNOT")
            return failure();

        Location loc = op.getLoc();
        Value ctrl = op.getInQubits()[0];
        Value tgt  = op.getInQubits()[1];

        // CNOT = (I ⊗ H) · CZ · (I ⊗ H)
        auto pi_2 = rewriter.create<arith::ConstantFloatOp>(
            loc, rewriter.getF64Type(), llvm::APFloat(M_PI_2));

        // H on target before CZ
        auto h1 = rewriter.create<CustomOp>(loc, mlir::TypeRange{tgt.getType()},
            ValueRange{pi_2}, ValueRange{tgt},
            rewriter.getStringAttr("RZ"), false, mlir::ValueRange{},
            mlir::ValueRange{});
        // ... (full H = RZ SX RZ, elided for brevity)

        auto cz = rewriter.create<CustomOp>(
            loc, mlir::TypeRange{ctrl.getType(), h1.getOutQubits()[0].getType()},
            ValueRange{},
            ValueRange{ctrl, h1.getOutQubits()[0]},
            rewriter.getStringAttr("CZ"), false, mlir::ValueRange{},
            mlir::ValueRange{});

        // H on target after CZ ... replaceOp with new ctrl+tgt values
        rewriter.replaceOp(op, cz.getOutQubits());
        return success();
    }
};

class IQMGateSetMapping
    : public mqss_catalyst::opt::IQMGateSetMappingPassBase<IQMGateSetMapping> {

    void runOnOperation() override {
        RewritePatternSet patterns(&getContext());
        
        // Register all your device-specific rewrite patterns
        patterns.add<HadamardToIQMPattern,
                     CNOTToIQMPattern>(& getContext());

        // GreedyRewrite applies patterns to fixpoint —
        // this is what --decompose-lowering lacks
        if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                                std::move(patterns)))) {
            signalPassFailure();
        }
    }
};

std::unique_ptr<mlir::Pass> mqss_catalyst::opt::IQMGateSetMappingPass() {
  return std::make_unique<IQMGateSetMapping>();
}