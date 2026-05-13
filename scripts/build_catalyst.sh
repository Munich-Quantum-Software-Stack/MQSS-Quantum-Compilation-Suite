#!/bin/bash

############# Pretty-print variables#################

#!/usr/bin/env bash

msg() {
  printf '%s\n' "$1"
}

blank() {
  printf '\n'
}

section() {
  blank
  printf '====== %s ======\n' "$1"
  blank
}

info() {
  printf '🔧 %s\n' "$1"
}

fail() {
  printf '❌ %s\n' "$1"
}


ok() {
  printf '✅ %s\n' "$1"
}

warn() {
  printf '⚠️ %s\n' "$1"
}

step() {
  printf '-> %s\n' "$1"
}

################## Architecture limitations ##################

ARCH="$(uname -m)"

case "$ARCH" in
  x86_64) CMAKE_ARCH="x86_64" ;;
  aarch64|arm64) CMAKE_ARCH="aarch64" ;;
  *)
    echo "Unsupported architecture: $ARCH" >&2
    exit 1
    ;;
esac

########################## Create build and deps directories #############################

CURRENT_DIR=$(pwd)
BUILD_DIR=${CURRENT_DIR}"/build"

DEPS_DIR=$CURRENT_DIR"/_deps/mqss-catalyst"
# Fetch the correct LLVM toolchain (21.1.8) and set the path to lib/cmake/llvm and lib/cmake/mlir
# Note: Using MQT's setup-mlir scripts to download the LLVM/MLIR setup.
#       Refer to https://github.com/munich-quantum-software/setup-mlir/ for more details
mkdir -p "${BUILD_DIR}"
mkdir -p "${DEPS_DIR}"
INSTALL_DIR="${BUILD_DIR}/bin"

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --build-docs)
      BUILD_DOCS=ON
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done


################## Download and Set Paths to external Dependencies #######################
section "Checking dependencies"
##### 1. Fetch the LLVM toolchain (16.0.6) and set the paths to cmake/llvm and cmake/mlir

info "Checking LLVM-21 toolchain"

LLVM_VERSION="21.1.8"
LLVM_BIN_DIR="${DEPS_DIR}/LLVM-${LLVM_VERSION}-toolchain"

#LLVM_BIN_DIR="/workspaces/compilers/MQSS-Catalyst-Compiler/mlir/llvm-project/build"

if [[ -e "$LLVM_BIN_DIR" ]]; then
    ok "Found LLVM-${LLVM_VERSION}-toolchain at: ${LLVM_BIN_DIR}"
else
    warn "LLVM-${LLVM_VERSION}-toolchain not found..."
    step "Downloading and installing it using https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh"
    curl -LsSf https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh | 
        bash -s -- -v ${LLVM_VERSION} -p ${LLVM_BIN_DIR}

    ok LLVM-${LLVM_VERSION}-toolchain downloaded and paths set
fi

LLVM_DIR="${LLVM_BIN_DIR}/lib/cmake/llvm"
MLIR_DIR="${LLVM_BIN_DIR}/lib/cmake/mlir"

info "Checking pennylane_catalyst-0.14.1 wheels"
CATALYST_PACAKGE="pennylane_catalyst-0.14.1-cp311-cp311-manylinux_2_28_$(uname -m).whl"
# Fetch OS specific catalyst installer. This is needed to get the catalyst python jit framework
VENV_DIR="${DEPS_DIR}/venv"
CATALYST_SITE_PACKAGE="${VENV_DIR}/lib/python3.11/site-packages/catalyst"

if [[ -e "${CATALYST_SITE_PACKAGE}" ]]; then
    info "Found Catalyst python package at: ${CATALYST_SITE_PACKAGE}"
else
    CATALYST_BINARY_REPO="https://github.com/PennyLaneAI/catalyst/releases/download/v0.14.1/${CATALYST_PACAKGE}"
    python3.11 -m pip install ${DEPS_DIR}/${CATALYST_PACAKGE} 
    wget -P $DEPS_DIR $CATALYST_BINARY_REPO
fi

if [[ -e "${LLVM_DIR}" && -e "${MLIR_DIR}" && -e "${CATALYST_SITE_PACKAGE}" ]]; then
    blank
    ok "All Required Dependencies Installed"
else
    warn "Failed to install all dependencies"
    exit 1
fi

################################## Build the project ###################################

section "Building MQSS-CATALYST"

BUILD_TYPE="Debug"
BUILD_CATALYST=ON

CATALYST_COMPILER_DIR="/workspaces/compilers/MQSS-Catalyst-Compiler"
PYTHON="${PYTHON:-$(which python3)}"
C_COMPILER="${C_COMPILER:-$(which gcc)}"
CXX_COMPILER="${CXX_COMPILER:-$(which g++)}"
COMPILER_LAUNCHER="${COMPILER_LAUNCHER:-$(which ccache)}"

if [[ "$(uname)" == "Darwin" ]]; then
    DEFAULT_ENABLE_LLD="OFF"
    SYMBOL_VISIBILITY="default"
else
    DEFAULT_ENABLE_LLD="ON"
    SYMBOL_VISIBILITY="default"
fi

ENABLE_LLD=${DEFAULT_ENABLE_LLD}
ENABLE_ZLIB=ON
ENABLE_ZSTD=OFF
ENABLE_ASAN=OFF
STRICT_WARNINGS=ON

if [[ "$ENABLE_ASAN" == "ON" ]]; then
    USE_SANITIZER_NAMES="Address"
    USE_SANITIZER_FLAGS="-fsanitize=address"
else
    USE_SANITIZER_NAMES=""
    USE_SANITIZER_FLAGS=""
fi

# LLVM_PROJECTS="mlir"
# LLVM_TARGETS="check-mlir llvm-symbolizer"

# Command to build the required LLVM version (21.0.0)
# cmake -G Ninja -S llvm-project/llvm -B $(LLVM_BUILD_DIR) \
# 	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
# 	-DLLVM_BUILD_EXAMPLES=OFF \
# 	-DLLVM_TARGETS_TO_BUILD="host" \
# 	-DLLVM_ENABLE_PROJECTS="$(LLVM_PROJECTS)" \
# 	-DLLVM_ENABLE_ASSERTIONS=ON \
# 	-DMLIR_ENABLE_BINDINGS_PYTHON=ON \
# 	-DPython_EXECUTABLE=$(PYTHON) \
# 	-DPython3_EXECUTABLE=$(PYTHON) \
# 	-DPython3_NumPy_INCLUDE_DIRS=$$($(PYTHON) -c "import numpy as np; print(np.get_include())") \
# 	-DCMAKE_C_COMPILER=$(C_COMPILER) \
# 	-DCMAKE_CXX_COMPILER=$(CXX_COMPILER) \
# 	-DCMAKE_C_COMPILER_LAUNCHER=$(COMPILER_LAUNCHER) \
# 	-DCMAKE_CXX_COMPILER_LAUNCHER=$(COMPILER_LAUNCHER) \
# 	-DLLVM_USE_SANITIZER=$(USE_SANITIZER_NAMES) \
# 	-DLLVM_ENABLE_LLD=$(ENABLE_LLD) \
# 	-DLLVM_ENABLE_ZLIB=$(ENABLE_ZLIB) \
# 	-DLLVM_ENABLE_ZSTD=$(ENABLE_ZSTD) \
# 	-DCMAKE_CXX_VISIBILITY_PRESET=$(SYMBOL_VISIBILITY)

cmake -G Ninja \
  -S "${CURRENT_DIR}/tools/mqss-catalyst" \
  -B "${CURRENT_DIR}/build/tools/mqss-catalyst" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_CATALYST="${BUILD_CATALYST}" \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DMLIR_DIR="${MLIR_DIR}" \
  -DLLVM_DIR="${LLVM_DIR}" \
  -DMLIR_LIB_DIR="${LLVM_BUILD_DIR}/lib" \
  -DCMAKE_C_COMPILER="${C_COMPILER}" \
  -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
  -DCMAKE_C_COMPILER_LAUNCHER="${COMPILER_LAUNCHER}" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="${COMPILER_LAUNCHER}" \
  -DLLVM_USE_SANITIZER="${USE_SANITIZER_NAMES}" \
  -DLLVM_ENABLE_LLD="${ENABLE_LLD}" \
  -DLLVM_ENABLE_ZLIB="${ENABLE_ZLIB}" \
  -DLLVM_ENABLE_ZSTD="${ENABLE_ZSTD}" \
  -DMQSS_BUILD_DIR="${CURRENT_DIR}/build" \
  -DINSTALL_DIR="${INSTALL_DIR}" \
  -DMQSS_SRC_DIR="${CURRENT_DIR}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DLLVM_EXTERNAL_LIT=$(which lit) \
  -DCATALYST_ENABLE_WARNINGS="${STRICT_WARNINGS}"


NUM_JOBS=4
blank
info "Building MQSS Catalyst Passes with ${NUM_JOBS} jobs..."

ninja -j"${NUM_JOBS}" -C "${CURRENT_DIR}/build/tools/mqss-catalyst"
# blank
# if [[ -e "${INSTALL_DIR}/bin/mqss-catalyst-opt" ]]; then
#     ok "Successfully installed MQSS-CATALYST"
# else
#     fail "MQSS-CATALYST not installed"
#     exit 1
# fi
