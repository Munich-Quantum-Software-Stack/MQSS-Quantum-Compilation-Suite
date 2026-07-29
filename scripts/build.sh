#!/usr/bin/env bash

############# Pretty-print variables#################

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
DEPS_DIR=$BUILD_DIR"/_deps"

# Create directories if they don't exist
mkdir -p "${BUILD_DIR}"
mkdir -p "${DEPS_DIR}"

CC="gcc"
CXX="g++"

# Default values
NUM_JOBS=4  # Default number of jobs
BUILD_DOCS=OFF  # Default: Do not build documentation
#BUILD_TYPE="Debug"  # Default: Release mode
INSTALL_DIR="${BUILD_DIR}/bin"

DEBUG=OFF
# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --debug)
      DEBUG=ON
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

##### 1. Fetch the LLVM toolchain (22.1.0) and set the paths to cmake/llvm and cmake/mlir


info "Checking LLVM-22.1.0 toolchain"

LLVM_VERSION="22.1.0"
LLVM_BIN_DIR="${DEPS_DIR}/LLVM-${LLVM_VERSION}-toolchain"

if [[ -e "$LLVM_BIN_DIR" ]]; then
    ok "Found LLVM-${LLVM_VERSION}-toolchain at: ${LLVM_BIN_DIR}"
else
    warn "LLVM-${LLVM_VERSION}-toolchain not found..."
    info "Downloading and installing it using https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh"
    curl -LsSf https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh |
        bash -s -- -v ${LLVM_VERSION} -p ${LLVM_BIN_DIR}

    ok LLVM-${LLVM_VERSION}-toolchain downloaded and paths set
fi

LLVM_DIR="${LLVM_BIN_DIR}/lib/cmake/llvm"
MLIR_DIR="${LLVM_BIN_DIR}/lib/cmake/mlir"


### 4. Check the installations

if [[ -e "${LLVM_DIR}" && -e "${MLIR_DIR}" ]]; then
    blank
    ok "All Required Dependencies Installed"
else
    warn "Failed to install all dependencies"
    exit 1
fi

################################## Configure the project ###################################

section "Configuring the MQSS-Quantum-Compilation Suite"

ok "Found LLVM DIR: ${LLVM_DIR}"
ok "Found MLIR DIR: ${MLIR_DIR}"

# echo "Configuring MQSS Passes Repository CMake..."
# cd  "${BUILD_DIR}"

cmake -G Ninja \
  -B "${BUILD_DIR}" \
  -DMQSS_USE_CUDAQ=ON \
  -DCUDAQ_AUTO_FETCH=ON \
  -DCATALYST_AUTO_FETCH=ON \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DMLIR_DIR="${MLIR_DIR}"\
  -DLLVM_DIR="${LLVM_DIR}"\
  -DMQSS_ENABLE_DEBUG="${DEBUG}" \
  -DINSTALL_DIR="${INSTALL_DIR}" \
  -DBUILD_DIR="${BUILD_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DLLVM_EXTERNAL_LIT=$(which lit) \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
