add_mlir_library(
  MQSSCITranslation
  QASM2ToQuake.cpp
  MLIRToTarget.cpp
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  MQSSCICodeGenPasses
  MQSSCITransformPasses
  MQSSCIPipelines
  MLIRIR
  MLIRFuncDialect
  MLIRPass
  MLIRTranslateLib)

target_include_directories(
  MQSSCITranslation PUBLIC $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR}/include>
                           $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR_BINARY}/include>)
