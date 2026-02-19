

#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"

#include "llvm/Support/raw_ostream.h"

#endif // EXTRACTOR_H

namespace mqss::opt {

    std::unique_ptr<mlir::Pass> createExtractorPass();

}