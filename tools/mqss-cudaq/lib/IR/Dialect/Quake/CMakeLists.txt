# ============================================================================ #
# Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                   #
# All rights reserved.                                                         #
#                                                                              #
# This source code and the accompanying materials are made available under     #
# the terms of the Apache License 2.0 which accompanies this distribution.     #
# ============================================================================ #

add_mlir_library(QuakeDialect
  QuakeDialect.cpp
  QuakeInterfaces.cpp
  QuakeOps.cpp
  QuakeTypes.cpp

  ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/tools/mqss-cudaq/include

  DEPENDS
  #MLIRQuakeDialectIncGen
  MLIRQuakeInterfacesIncGen
  MLIRQuakeOpsIncGen
  MLIRQuakeTypesIncGen
  MLIRQuakeDialectIncGen
  
  LINK_LIBS
  CCDialect
  MLIRFuncDialect
  MLIRIR
)

