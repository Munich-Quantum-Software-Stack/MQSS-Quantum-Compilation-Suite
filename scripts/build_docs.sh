#!/usr/bin/env bash

# Note: To be invoked via root/Makefile
CURRENT_DIR=$(pwd)

cmake -G Ninja
  -S "${CURRENT_DIR}/docs" \
  -B "${CURRENT_DIR}/build/docs"