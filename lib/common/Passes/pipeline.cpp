
#include "include/transforms/PassUtils.h"

using namespace mlir;

void mqss_backend::O1(mlir::OpPassManager &pm) {

  CommonMappingPassOptions MappingOpts;
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O2(mlir::OpPassManager &pm) {

  // MQSS MLIR Passes
  CommonGateCancellationPassOptions CancelOpts;
  CommonCommutePassOptions CommuteOpts;
  CancelOpts.mode = "CancelGate";
  CommuteOpts.mode = "CX-RX";

  pm.addPass(createCommonGateCancellationPass(CancelOpts));
  pm.addPass(CommonCNOTReversePass());
  pm.addPass(createCommonCommutePass(CommuteOpts));

  // Standard MLIR Passes
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O3(mlir::OpPassManager &pm) {

  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}