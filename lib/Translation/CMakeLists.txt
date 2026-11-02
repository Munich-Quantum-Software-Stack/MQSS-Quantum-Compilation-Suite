add_mlir_library(
  MQSSCITranslation
  QASM2ToQuake.cpp
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  MLIRIR
  MLIRFuncDialect
  MLIRTranslateLib)

target_include_directories(
  MQSSCITranslation PUBLIC $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR}/include>
                           $<BUILD_INTERFACE:${MQSSCI_ROOT_DIR_BINARY}/include>)
