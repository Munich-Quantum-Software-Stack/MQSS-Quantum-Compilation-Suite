add_mlir_library(
  MQSSCIPipelines
  Pipeline.cpp
  DEPENDS
  MQSSMQSSCodeGenPassIncGen
  MQSSTransformsPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  MLIRPass
  MQSSCICodeGenPasses
  MQSSCITransformPasses
  MLIRTransforms)

target_compile_options(MQSSCIPipelines PRIVATE -fexceptions)

if(TARGET obj.MQSSCIPipelines)
  target_compile_options(obj.MQSSCIPipelines PRIVATE -fexceptions)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(obj.MQSSCIPipelines PRIVATE MQSS_ENABLE_DEBUG)
  endif()
endif()
