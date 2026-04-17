#!/bin/bash

CURRENT_DIR=$(pwd)

CATALYST_COMPILER_DIR="/workspaces/compilers/MQSS-Catalyst-Compiler"


# Fetch the correct LLVM toolchain (21.1.8) and set the path to lib/cmake/llvm and lib/cmake/mlir
# Note: Using MQT's setup-mlir scripts to download the LLVM/MLIR setup.
#       Refer to https://github.com/munich-quantum-software/setup-mlir/ for more details

# cd $DEPS_DIR
# curl -LsSf https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh | 
#       bash -s -- -v 21.1.8 -p /workspaces/temp/llvm-21-toolchain

LLVM_BUILD_DIR="/workspaces/temp/llvm-21-toolchain"
LLVM_DIR="${LLVM_BUILD_DIR}/lib/cmake/llvm"
MLIR_DIR="${LLVM_BUILD_DIR}/lib/cmake/mlir"

# Fetch OS specific cuda-quantum installer. This is needed to get the catalyst python jit framework
# CATALYST_BINARY_REPO="https://github.com/PennyLaneAI/catalyst/releases/download/v0.14.1/pennylane_catalyst-0.14.1-cp311-cp311-manylinux_2_28_$(uname -m).whl"
# Install command: python3 -m pip install ./pennylane_catalyst-0.14.1-cp311-cp311-manylinux_2_28_$(uname -m).whl 
# export <path to bin/catalyst>


BUILD_TYPE="Debug"
BUILD_CATALYST=ON

source "${CATALYST_COMPILER_DIR}/catalyst-venv/bin/activate"                      # Edit in the future

PYTHON="${PYTHON:-$(which python3)}"
C_COMPILER="${C_COMPILER:-$(which clang)}"
CXX_COMPILER="${CXX_COMPILER:-$(which clang++)}"
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
  -DQUANTUM_ENABLE_BINDINGS_PYTHON=ON \
  -DPython3_EXECUTABLE="${PYTHON}" \
  -DPython3_NumPy_INCLUDE_DIRS="$($PYTHON -c 'import numpy as np; print(np.get_include())')" \
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
  -DMQSS_SRC_DIR="${CURRENT_DIR}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCATALYST_ENABLE_WARNINGS="${STRICT_WARNINGS}"


NUM_JOBS=4
ninja -j"${NUM_JOBS}" -C "${CURRENT_DIR}/build/tools/mqss-catalyst"