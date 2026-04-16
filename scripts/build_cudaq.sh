#!/bin/bash

# Define directories
CURRENT_DIR=$(pwd)

INSTALL_PATH="${INSTALL_PATH:-$HOME/.passes}"
# Default values
NUM_JOBS=4  # Default number of jobs
BUILD_DOCS=OFF  # Default: Do not build documentation
BUILD_TESTS=OFF  # Default: Do not build tests
BUILD_Quake=OFF
BUILD_CUDAQ=ON
BUILD_TYPE="Debug"  # Default: Release mode

# Default directories (can be overridden by arguments)
MLIR_DIR="/usr/local/llvm/lib/cmake/mlir"
LLVM_DIR="/usr/local/llvm/lib/cmake/llvm"
INSTALL_DIR="${INSTALL_PATH:-$HOME/.passes}"

CUDAQ_DIR="/workspaces/executables/cudaq"

# Fetch OS specific cuda-quantum installer
# CUDAQ_BINARY_REPO="https://github.com/NVIDIA/cuda-quantum/releases/download/0.14.0/install_cuda_quantum_cu13.$(uname -m)"
# Install command: bash install_cuda_quantum*.$(uname -m) --accept -- --installpath <my/path/>
# export <path to bin/cudaq-quake>
# export <path to bin/cudaq-opt>

CC="gcc"
CXX="g++"

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -j|--jobs)
      NUM_JOBS="$2"
      shift 2
      ;;
		--debug)
    	BUILD_TYPE="Debug"
    	shift
    	;;
    --mlir-dir)
      MLIR_DIR="$2"
      shift 2
      ;;
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --clang-dir)
      CLANG_DIR="$2"
      shift 2
      ;;
    --llvm-dir)
      LLVM_DIR="$2"
      shift 2
      ;;
    --build-quake)
      BUILD_Quake=ON
      shift
      ;;
    --build-docs)
      BUILD_DOCS=ON
      shift
      ;;
    --build-tests)
      BUILD_TESTS=ON
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

BUILD_DIR=${CURRENT_DIR}"/build"

# Create directories if they don't exist
mkdir -p "${BUILD_DIR}"

cd  "${BUILD_DIR}" || { echo "Failed to navigate back to the original directory."; exit 1; }

echo "Configuring MQSS Passes Repository CMake..."

cmake -G Ninja \
  -S "${CURRENT_DIR}/tools/mqss-cudaq" \
  -B "${CURRENT_DIR}/build/tools/mqss-cudaq" \
  -DBUILD_CUDAQ="${BUILD_CUDAQ}"\
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DMLIR_DIR="${MLIR_DIR}"\
  -DLLVM_DIR="${LLVM_DIR}"\
  -DBUILD_MLIR_PASSES_DOCS="${BUILD_DOCS}" \
  -DCUDAQ_SOURCE_DIR="${CUDAQ_DIR}" \
  -DMQSS_SRC_DIR="${CURRENT_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
echo "Building MQSS Repository Passes with ${NUM_JOBS} jobs..."
ninja -j"${NUM_JOBS}" -C "${CURRENT_DIR}/build/tools/mqss-cudaq"
echo "SUCCESSFULL!"
echo " "
