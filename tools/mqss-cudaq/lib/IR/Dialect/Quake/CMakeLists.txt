
add_mlir_library(QuakeDialect
  QuakeDialect.cpp
  QuakeInterfaces.cpp
  QuakeOps.cpp
  QuakeTypes.cpp

  DEPENDS
  mqss-cudaq-headers
  QuakeInterfacesIncGen
  QuakeDialectIncGen
  QuakeOpsIncGen
  QuakeTypesIncGen
  

  LINK_LIBS
  CCDialect
  MLIRFuncDialect
  MLIRIR
)

