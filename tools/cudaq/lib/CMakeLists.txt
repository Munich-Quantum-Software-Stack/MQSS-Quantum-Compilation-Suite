add_subdirectory(Passes)
# set(PASS_ANALYSIS_SOURCES
#   Passes/Analysis/PrintQuakeGates.cpp
# )

# add_mlir_library(MLIRPasses
#   SHARED
#   ${PASS_ANALYSIS_SOURCES}

#   LINK_LIBS PUBLIC
#     MLIRTensorDialect
#     MLIRIR
#     MLIRPass
#     MLIRTransforms
# )


# target_include_directories(MLIRPasses
#   PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
# )

