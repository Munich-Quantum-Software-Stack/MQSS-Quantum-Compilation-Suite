/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/



#include "include/PassUtils.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace llvm;


#ifdef BUILD_CUDAQ_ENABLED
namespace mqss_backend = mqss_cudaq::opt;

namespace mqss_cudaq::opt {
#define GEN_PASS_CLASSES
#include "MQSSCUDAQPasses/Transforms.h.inc"

} // namespace mqss_cudaq::opt
#endif

#ifdef BUILD_CATALYST_ENABLED
namespace mqss_backend = mqss_catalyst::opt;

namespace mqss_catalyst::opt {
#define GEN_PASS_CLASSES
#include "MQSSCatalystPasses/Transforms.h.inc"

} // namespace mqss_catalyst::opt
#endif

namespace {

  enum class PassMode {
    HXHToZ,
    HZHToX,
    NA
};

struct SharedPassLogic{

  void run(llvm::DenseMap<func::FuncOp, QuantumOpInfo> KernelDialectInfo, PassMode Mode) {

    ReductionPassInfoty PassInfo;
    if (Mode == PassMode::HXHToZ) {

      PassInfo.GatesToCancel.push_back(H);
      PassInfo.GatesToCancel.push_back(PauliX);
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.NewGateTy = Gate::PauliZ;
      PassInfo.CompareKey = {"Target", "Target"};
    } 
    else if (Mode == PassMode::HZHToX) {
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.GatesToCancel.push_back(PauliZ);
      PassInfo.GatesToCancel.push_back(H);
      PassInfo.NewGateTy = Gate::PauliX;
      PassInfo.CompareKey = {"Target", "Target"};
    }

    for (auto &[kernel, QInfo] : KernelDialectInfo) {
      performReduction(QInfo, PassInfo);
    }
  }
};

class CommonReduction
    : public mqss_backend::CommonReductionPassBase<CommonReduction> {

public:
    using Base = mqss_backend::CommonReductionPassBase<CommonReduction>;
    using Base::Base;

  void runOnOperation() override {
#ifdef BUILD_CUDAQ_ENABLED
    auto &analysis = getAnalysis<QuakeAnalysis>();
#endif
#ifdef BUILD_CATALYST_ENABLED
    auto &analysis = getAnalysis<CatalystQuantumAnalysis>();
#endif

    llvm::outs() << "\n[Applying Pass: CommonReduction]\n";

    SharedPassLogic PassLogic;

    if(mode == "HZHToX"){
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::HZHToX);
    }
    else if(mode == "HXHToZ")
      PassLogic.run(analysis.getKernelDialectInfo(), PassMode::HXHToZ);
    else{
      getOperation()->emitError() << "invalid mode: " << mode;
      signalPassFailure();
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};


} // namespace

#ifdef BUILD_CUDAQ_ENABLED
std::unique_ptr<mlir::Pass> mqss_cudaq::opt::CommonReductionPass() {
  return std::make_unique<CommonReduction>();
}
#endif

#ifdef BUILD_CATALYST_ENABLED
std::unique_ptr<mlir::Pass> mqss_catalyst::opt::CommonReductionPass() {
  return std::make_unique<CommonReduction>();
}
#endif