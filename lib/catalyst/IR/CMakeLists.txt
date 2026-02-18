add_mlir_library(MLIRQuantum
    QuantumDialect.cpp
    QuantumInterfaces.cpp
    QuantumOps.cpp

    ADDITIONAL_HEADER_DIRS
    ${CURRENT_DIR}/include/Catalyst

    DEPENDS
    MLIRQuantumInterfacesIncGen
    MLIRQuantumOpsIncGen
    MLIRQuantumEnumsIncGen
)
