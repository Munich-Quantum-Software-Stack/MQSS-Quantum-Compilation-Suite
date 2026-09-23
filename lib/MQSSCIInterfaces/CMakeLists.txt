add_mlir_library(MQSSCIInterfaces MQSSCompiler.cpp LINK_LIBS PUBLIC
                 MQSSCITransformPasses MQSSCIVerificationPasses)

target_compile_options(MQSSCIInterfaces PRIVATE -fexceptions)
if(TARGET obj.MQSSCIInterfaces)
  target_compile_options(obj.MQSSCIInterfaces PRIVATE -fexceptions)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(obj.MQSSCIInterfaces PRIVATE MQSS_ENABLE_DEBUG)
  endif()
endif()
