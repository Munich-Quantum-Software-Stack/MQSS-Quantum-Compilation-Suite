
# include_directories(
#   # 1. The source 'include' dir (so it finds QuantumDialect.h)
#   ${DIALECTS_SRC_DIR}/tools/catalyst/include
# )

add_mlir_library(MLIRQuantum
    QuantumDialect.cpp
    QuantumInterfaces.cpp
    QuantumOps.cpp
    
    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/tools/mqss-catalyst/include

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

# target_include_directories(MLIRQuantum
#   PUBLIC
#     ${CATALYST_MAIN_INCLUDE_DIR}
#     ${PROJECT_BINARY_DIR}/tools/catalyst/include
# )
