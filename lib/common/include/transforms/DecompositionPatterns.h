

// // The rewrite pattern body — written against YOUR dialect ops
// struct HToPhasedRxPattern {
//   using OpRewritePattern::OpRewritePattern;

//   mlir::LogicalResult matchAndRewrite(your_dialect::HOp op,
//                                        mlir::PatternRewriter &rewriter) const override {
//     if (!op.getControls().empty()) return mlir::failure();

//     auto loc = op->getLoc();
//     Value target = op.getTarget();

//     Value pi   = createFloatConst(loc, M_PI,   rewriter);
//     Value pi_2 = createFloatConst(loc, M_PI_2, rewriter);
//     Value zero = createFloatConst(loc, 0.0,    rewriter);

//     // phased_rx(π/2, π/2) target
//     your_dialect::PhasedRxOp::create(rewriter, loc, {pi_2, pi_2}, target);
//     // phased_rx(π, 0) target
//     your_dialect::PhasedRxOp::create(rewriter, loc, {pi, zero},   target);

//     rewriter.eraseOp(op);
//     return mlir::success();
//   }
// };

// // The metadata struct — what the graph sees
// struct HToPhasedRxMetadata : public PatternMetadata {
//   HToPhasedRxMetadata() {
//     name      = "HToPhasedRx";
//     sourceOp  = OperatorInfo("h");           // { name="h", numControls=0 }
//     targetOps = { OperatorInfo("phased_rx") }; // { name="phased_rx", numControls=0 }
//   }

//   std::unique_ptr<mlir::RewritePattern>
//   create(mlir::MLIRContext *ctx,
//          mlir::PatternBenefit benefit,
//          llvm::ArrayRef<std::size_t> disabledCtrlCounts) const override {
//     return std::make_unique<HToPhasedRxPattern>(ctx, benefit);
//   }
// };