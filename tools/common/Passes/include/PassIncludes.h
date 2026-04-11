

#ifdef BUILD_CUDAQ_ENABLED
#include "MQSSCUDAQPasses/Transforms.hpp"
#include "SemanticExtractLayer/QuakeExtractor.h"

namespace mqss_backend = mqss_cudaq::opt;

#define DialectAnalysis QuakeAnalysis

namespace mqss_cudaq::opt {
#define GEN_PASS_CLASSES
#include "MQSSCUDAQPasses/Transforms.h.inc"

} // namespace mqss_cudaq::opt
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "MQSSCatalystPasses/Transforms.h"
#include "SemanticExtractLayer/CatalystExtractor.h"

namespace mqss_backend = mqss_catalyst::opt;
#define DialectAnalysis CatalystQuantumAnalysis

namespace mqss_catalyst::opt {
#define GEN_PASS_CLASSES
#include "MQSSCatalystPasses/Transforms.h.inc"

} // namespace mqss_catalyst::opt
#endif
