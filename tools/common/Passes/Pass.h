

#ifdef BUILD_CUDAQ_ENABLED
#include "MQSSCUDAQPasses/Transforms.hpp"
#include "../SemanticExtractLayer/QuakeExtractor.h"
#endif

#ifdef BUILD_CATALYST_ENABLED
#include "MQSSCatalystPasses/Transforms.hpp"
#include "../SemanticExtractLayer/CatalystExtractor.h"
#endif