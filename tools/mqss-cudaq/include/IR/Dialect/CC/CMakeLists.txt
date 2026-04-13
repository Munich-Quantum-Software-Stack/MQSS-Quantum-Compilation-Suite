# ============================================================================ #
# Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                   #
# All rights reserved.                                                         #
#                                                                              #
# This source code and the accompanying materials are made available under     #
# the terms of the Apache License 2.0 which accompanies this distribution.     #
# ============================================================================ #

#add_mlir_dialect(Quake quake)
add_mlir_interface(CCInterfaces)

set(LLVM_TARGET_DEFINITIONS CCOps.td)
mlir_tablegen(CCOps.h.inc -gen-op-decls)
mlir_tablegen(CCOps.cpp.inc -gen-op-defs)
add_public_tablegen_target(MLIRCCOpsIncGen)


set(LLVM_TARGET_DEFINITIONS CCTypes.td)
mlir_tablegen(CCTypes.h.inc -gen-typedef-decls -typedefs-dialect=cc)
mlir_tablegen(CCTypes.cpp.inc -gen-typedef-defs -typedefs-dialect=cc)
add_public_tablegen_target(MLIRCCTypesIncGen)

set(LLVM_TARGET_DEFINITIONS CCDialect.td)
mlir_tablegen(CCDialect.h.inc -gen-dialect-decls -dialect=cc)
mlir_tablegen(CCDialect.cpp.inc -gen-dialect-defs -dialect=cc)
add_public_tablegen_target(MLIRCCDialectIncGen)

# set(LLVM_TARGET_DEFINITIONS QuakeTypes.td)
# mlir_tablegen(QuakeTypes.h.inc -gen-typedef-decls)
# mlir_tablegen(QuakeTypes.cpp.inc -gen-typedef-defs)
#add_public_tablegen_target(MLIRQuakeTypesIncGen)

include_directories(${CMAKE_CURRENT_BINARY_DIR})