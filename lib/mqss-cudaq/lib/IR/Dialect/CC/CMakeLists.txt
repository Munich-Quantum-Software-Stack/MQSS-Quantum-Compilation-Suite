
add_mlir_library(CCDialect
  CCDialect.cpp
  CCOps.cpp
  CCTypes.cpp

  DEPENDS
  mqss-cudaq-headers
  CCInterfacesIncGen
  CCDialectIncGen
  CCOpsIncGen
  CCTypesIncGen


  LINK_LIBS
    MLIRComplexDialect
    MLIRFuncDialect
    MLIRLLVMDialect
    MLIRIR
)
