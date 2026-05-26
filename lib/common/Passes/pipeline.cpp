

#include "include/transforms/PassUtils.h"

using namespace mlir;

void mqss_backend::O1(mlir::PassManager &pm) {
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O2(mlir::PassManager &pm) {

  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}

void mqss_backend::O3(mlir::PassManager &pm) {

  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}