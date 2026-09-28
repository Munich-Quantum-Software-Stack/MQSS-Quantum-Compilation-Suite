/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

// A deliberately narrow port of cudaq's own `expand-measurements` pass
// (cudaq/lib/Optimizer/Transforms/ExpandMeasurements.cpp), scoped to exactly
// what MQSSCI circuits need: turning `quake.mz` on a whole (statically-sized)
// qubit register into one `quake.mz` per qubit, so lowering paths with no
// concept of a "measure this whole register" instruction -- IQM JSON's
// `translateToIQMJson` in particular -- have something they can translate.
//
// cudaq's own version is general-purpose: it also handles `mx`/`my`,
// `quake.reset`, `!cc.measure_handle`-typed results, and registers whose size
// isn't known until runtime -- which it supports by emitting a genuine
// `cc.loop` (driven by `quake.veq_size`) that only collapses back down to
// flat per-qubit ops after a further loop-unrolling pass this project doesn't
// have. Nothing in MQSSCI produces `mx`/`my`, `quake.reset`, or a
// dynamically-sized `!quake.veq`, so none of that generality is needed here:
// every `!quake.veq<N>` this pipeline sees already carries its size N in the
// type itself, so this version unrolls directly at rewrite time -- plain
// `quake.extract_ref`/`quake.mz` pairs, no loop, no `quake.veq_size`, nothing
// left over for a later pass to clean up.
#include "Passes/Transforms/TransformPasses.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"

#include <cudaq/Optimizer/Dialect/CC/CCDialect.h>
#include <cudaq/Optimizer/Dialect/CC/CCOps.h>
#include <cudaq/Optimizer/Dialect/Quake/QuakeDialect.h>
#include <cudaq/Optimizer/Dialect/Quake/QuakeOps.h>
#include <cudaq/Optimizer/Dialect/Quake/QuakeTypes.h>

namespace mqss::mqssci::opt {

#define GEN_PASS_DEF_EXPANDMEASUREMENTS
#include "Passes/Transforms/TransformPasses.h.inc"

} // namespace mqss::mqssci::opt

using namespace mlir;

namespace {

// Expands a `quake.mz` whose sole target is a statically-sized `!quake.veq`
// into one `quake.mz` per qubit. Only handles the two consumer shapes MQSSCI
// circuits actually produce: no consumer at all, or `quake.discriminate`
// (the `!cc.stdvec<!quake.measure>` -> `!cc.stdvec<i1>` bridge OpenQASM2/QIR
// lowering expect). Anything else -- multiple/mixed targets, a
// dynamically-sized register, a non-discriminate consumer -- is out of scope
// and left to the dynamic legality check below to reject with a clear
// diagnostic, rather than silently mishandled.
class ExpandVeqMeasurement : public OpRewritePattern<cudaq::quake::MzOp> {
public:
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(cudaq::quake::MzOp measureOp,
                                PatternRewriter &rewriter) const override {
    auto targets = measureOp.getTargets();
    if (targets.size() != 1)
      return failure();
    auto veqTy = dyn_cast<cudaq::quake::VeqType>(targets[0].getType());
    if (!veqTy || !veqTy.hasSpecifiedSize())
      return failure();

    // Classify consumers before mutating anything: if an unsupported one
    // turns up, bail out with the IR untouched instead of partially
    // rewriting and then failing.
    SmallVector<cudaq::quake::DiscriminateOp> discUsers;
    for (auto *u : measureOp.getMeasOut().getUsers()) {
      auto disc = dyn_cast<cudaq::quake::DiscriminateOp>(u);
      if (!disc)
        return failure();
      discUsers.push_back(disc);
    }

    auto loc = measureOp.getLoc();
    auto *ctx = rewriter.getContext();
    Value veq = targets[0];
    std::size_t numQubits = veqTy.getSize();
    auto measTy = cudaq::quake::MeasureType::get(ctx);
    auto i1Ty = rewriter.getI1Type();

    SmallVector<Value> discBits;
    for (std::size_t i = 0; i < numQubits; ++i) {
      Value qv = cudaq::quake::ExtractRefOp::create(rewriter, loc, veq, i);
      auto meas = cudaq::quake::MzOp::create(rewriter, loc, measTy, qv);
      if (auto registerName = measureOp.getRegisterNameAttr())
        meas.setRegisterName(registerName);
      if (!discUsers.empty())
        discBits.push_back(cudaq::quake::DiscriminateOp::create(
            rewriter, loc, i1Ty, meas.getMeasOut()));
    }

    if (discUsers.empty()) {
      rewriter.eraseOp(measureOp);
      return success();
    }

    // Fold the per-qubit bits into a `!cc.stdvec<i1>` for each
    // `quake.discriminate` consumer. N is a compile-time constant, so this
    // is a fixed-size buffer filled by straight-line stores at constant
    // offsets -- no loop needed to build or to later unroll.
    auto i8Ty = rewriter.getI8Type();
    Value sizeVal = arith::ConstantIntOp::create(rewriter, loc, numQubits, 64);
    Value buf = cudaq::cc::AllocaOp::create(rewriter, loc, i8Ty, sizeVal);
    for (std::size_t i = 0; i < numQubits; ++i) {
      Value idx = arith::ConstantIntOp::create(rewriter, loc, i, 64);
      auto addr = cudaq::cc::ComputePtrOp::create(
          rewriter, loc, cudaq::cc::PointerType::get(i8Ty), buf, idx);
      auto byte = cudaq::cc::CastOp::create(rewriter, loc, i8Ty, discBits[i],
                                            cudaq::cc::CastOpMode::Unsigned);
      cudaq::cc::StoreOp::create(rewriter, loc, byte, addr);
    }
    auto stdvecI1Ty = cudaq::cc::StdvecType::get(ctx, i1Ty);
    auto ptrArrI1Ty =
        cudaq::cc::PointerType::get(cudaq::cc::ArrayType::get(i1Ty));
    auto buffCast = cudaq::cc::CastOp::create(rewriter, loc, ptrArrI1Ty, buf);
    for (auto disc : discUsers)
      rewriter.replaceOpWithNewOp<cudaq::cc::StdvecInitOp>(disc, stdvecI1Ty,
                                                           buffCast, sizeVal);

    rewriter.eraseOp(measureOp);
    return success();
  }
};

class ExpandMeasurementsPass
    : public mqss::mqssci::opt::impl::ExpandMeasurementsBase<
          ExpandMeasurementsPass> {
public:
  using Base::Base;
  void runOnOperation() override {
    auto *op = getOperation();
    auto *ctx = &getContext();
    RewritePatternSet patterns(ctx);
    patterns.insert<ExpandVeqMeasurement>(ctx);
    ConversionTarget target(*ctx);
    target.addLegalDialect<cudaq::quake::QuakeDialect, cudaq::cc::CCDialect,
                           arith::ArithDialect, LLVM::LLVMDialect>();
    // Legal as-is iff already a scalar (per-qubit) measurement; anything
    // else must go through ExpandVeqMeasurement, which either handles it or
    // (for an out-of-scope shape) leaves it illegal so this reports a clear
    // conversion failure instead of silently shipping an untranslatable op.
    target.addDynamicallyLegalOp<cudaq::quake::MzOp>([](cudaq::quake::MzOp x) {
      return isa<cudaq::quake::MeasureType>(x.getMeasOut().getType());
    });
    if (failed(applyPartialConversion(op, target, std::move(patterns)))) {
      op->emitOpError("could not expand measurements");
      signalPassFailure();
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::createExpandMeasurementsPass() {
  return std::make_unique<ExpandMeasurementsPass>();
}
