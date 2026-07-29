#!/usr/bin/env bash

# Note: To be invoked via root/Makefile
CURRENT_DIR=$(pwd)
BUILD_DOCS=ON

echo "$CURRENT_DIR"
cmake -G Ninja \
  -S "${CURRENT_DIR}/docs" \
  -DMQSS_SRC_DIR="${CURRENT_DIR}" \
  -DBUILD_MLIR_PASSES_DOCS="${BUILD_DOCS}" \
  -B "${CURRENT_DIR}/build/docs"
