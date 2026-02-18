
include_directories(
  # 1. The source 'include' dir (so it finds QuantumDialect.h)
  ${DIALECTS_SRC_DIR}/tools/catalyst/include
  
  # 2. The build 'include' dir (so it finds "IR/QuantumOpsDialect.h.inc")
  ${DIALECTS_SRC_DIR}/tools/catalyst/include
)

add_mlir_library(MLIRQuantum
    QuantumDialect.cpp
    QuantumInterfaces.cpp
    QuantumOps.cpp
    
    ADDITIONAL_HEADER_DIRS
    ${CURRENT_DIR}/tools/catalyst/include
    ${CMAKE_CURRENT_SOURCE_DIR} 

    DEPENDS
    MLIRQuantumInterfacesIncGen
    MLIRQuantumOpsIncGen
    MLIRQuantumEnumsIncGen

    # 3. Link against MLIR core libraries you need
    LINK_LIBS PUBLIC
    MLIRIR
    MLIRBytecodeReader
    MLIRSideEffectInterfaces
)
