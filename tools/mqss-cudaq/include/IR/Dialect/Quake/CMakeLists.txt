# ============================================================================ #
# Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                   #
# All rights reserved.                                                         #
#                                                                              #
# This source code and the accompanying materials are made available under     #
# the terms of the Apache License 2.0 which accompanies this distribution.     #
# ============================================================================ #

#add_mlir_dialect(Quake quake)
add_mlir_interface(QuakeInterfaces)

set(LLVM_TARGET_DEFINITIONS QuakeOps.td)
mlir_tablegen(QuakeOps.h.inc -gen-op-decls)
mlir_tablegen(QuakeOps.cpp.inc -gen-op-defs)
add_public_tablegen_target(MLIRQuakeOpsIncGen)


set(LLVM_TARGET_DEFINITIONS QuakeTypes.td)
mlir_tablegen(QuakeTypes.h.inc -gen-typedef-decls -typedefs-dialect=quake)
mlir_tablegen(QuakeTypes.cpp.inc -gen-typedef-defs -typedefs-dialect=quake)
add_public_tablegen_target(MLIRQuakeTypesIncGen)

set(LLVM_TARGET_DEFINITIONS QuakeDialect.td)
mlir_tablegen(QuakeDialect.h.inc -gen-dialect-decls -dialect=quake)
mlir_tablegen(QuakeDialect.cpp.inc -gen-dialect-defs -dialect=quake)
add_public_tablegen_target(MLIRQuakeDialectIncGen)

# set(LLVM_TARGET_DEFINITIONS QuakeTypes.td)
# mlir_tablegen(QuakeTypes.h.inc -gen-typedef-decls)
# mlir_tablegen(QuakeTypes.cpp.inc -gen-typedef-defs)
#add_public_tablegen_target(MLIRQuakeTypesIncGen)

include_directories(${CMAKE_CURRENT_BINARY_DIR})