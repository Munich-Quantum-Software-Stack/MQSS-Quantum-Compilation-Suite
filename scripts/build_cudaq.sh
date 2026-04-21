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
  printf '[INFO] %s\n' "$1"
}

ok() {
  printf '[OK]   %s\n' "$1"
}

warn() {
  printf '[WARN] %s\n' "$1"
}

step() {
  printf '  -> %s\n' "$1"
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
INSTALL_DIR="${BUILD_DIR}"

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

CLANG_LLVM_VERSION=""
if [[ "$ARCH" == "aarch64" ]]; then
  CLANG_LLVM_VERSION="clang+llvm-16.0.4-${ARCH}-linux-gnu"
  LLVM_LIB_DIR="${DEPS_DIR}/${CLANG_LLVM_VERSION}/lib"
else
  CLANG_LLVM_VERSION="clang+llvm-16.0.4-${ARCH}-linux-gnu-ubuntu-22.04"
  LLVM_LIB_DIR="${DEPS_DIR}/${CLANG_LLVM_VERSION}/lib"
fi

if [[ -e "$LLVM_LIB_DIR" ]]; then
  ok "Found LLVM tool-chain at: ${LLVM_LIB_DIR}"
else
  warn "LLVM_LIB_DIR not found..."
  step "Downloading ${CLANG_LLVM_VERSION}.tar.xz"
  wget -P $DEPS_DIR \
    https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.4/${CLANG_LLVM_VERSION}.tar.xz
  
  ok "Unpacking LLVM-toolchain..."
  tar -xJf $DEPS_DIR/${CLANG_LLVM_VERSION}.tar.xz -C $DEPS_DIR
fi

#LLVM_LIB_DIR="/usr/local/llvm/lib"

MLIR_DIR="${LLVM_LIB_DIR}/cmake/mlir"
LLVM_DIR="${LLVM_LIB_DIR}/cmake/llvm"

#### 2. Fetch the correct cuda_quantum installer and set path to CUDA_QUAKE 

mkdir -p "${DEPS_DIR}/cudaq"
CUDAQ_DIR="${DEPS_DIR}/cudaq"
# Note: $CUDAQ_DIR is passed as an input argument to cmake (at the bottom) when building the project
#       $CUDAQ_DIR should already contain the all the required header files for the project (e.g. cudaq.h)

CUDA_QUAKE="${DEPS_DIR}/cudaq/bin/cudaq-quake"
# Fetch OS specific cuda-quantum installer
if [[ -e "$CUDA_QUAKE" ]]; then
  ok "Found CUDAQ Binaries at: ${DEPS_DIR}/cudaq/bin"
else
  warn "CUDAQ Binaries NOT FOUND!"
  step "Downloading and Installing CUDAQ Assets..."
  wget -P $DEPS_DIR "https://github.com/NVIDIA/cuda-quantum/releases/download/0.14.0/install_cuda_quantum_cu13.$(uname -m)"
  bash $DEPS_DIR/install_cuda_quantum*.$(uname -m) --accept -- --installpath "${DEPS_DIR}/cudaq"
fi
################################## CMAKE dependence ###################################

MIN_VER="3.19.0"
MAX_VER="3.30.0"
INSTALL_VER="3.29.0"

CMAKE_DIR="${DEPS_DIR}/cmake-${INSTALL_VER}-linux-${CMAKE_ARCH}"
CMAKE_BIN="${DEPS_DIR}/cmake-3.29.0-linux-${CMAKE_ARCH}/bin/cmake"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz"

version_ge() {
  # true if $1 >= $2
  [[ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" == "$2" ]]
}

version_lt() {
  # true if $1 < $2
  ! version_ge "$1" "$2"
}

cmake_ok=false
current_ver=""

if command -v cmake >/dev/null 2>&1; then
  current_ver="$(cmake --version | awk 'NR==1 {print $3}')"
  info "System CMake version: ${current_ver}"

  if version_ge "$current_ver" "$MIN_VER" && version_lt "$current_ver" "$MAX_VER"; then
    cmake_ok=true
    warn "CMake version is within the allowed range [${MIN_VER}, ${MAX_VER}]."
  else
    warn " System CMake version is outside the allowed range [${MIN_VER}, ${MAX_VER}]."
  fi
else
  warn "CMake not found."
fi

if [[ "$cmake_ok" != true ]]; then
  info "Checking if cmake-$INSTALL_VER is already installed in $DEPS_DIR"
  if [[ -e "$CMAKE_BIN" ]]; then
    ok "Found CMake at: ${CMAKE_BIN}"
    export PATH="${DEPS_DIR}/cmake-3.29/bin:$PATH"
  else
    blank
    info "Installing CMake ${INSTALL_VER} ..."
    info "Download URL: ${CMAKE_URL}"
    info "Install path: ${CMAKE_DIR}"
    blank

    wget -P $DEPS_DIR $CMAKE_URL
    tar -xzf "${DEPS_DIR}/cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz" -C "$DEPS_DIR"

    if [[ ! -x "$CMAKE_BIN" ]]; then
      warn "Installation failed: ${CMAKE_BIN} not found." >&2
      exit 1
    fi

    ok "Installed CMake at: $CMAKE_BIN"
    info "Version: $("$CMAKE_BIN" --version | awk 'NR==1 {print $3}')"
    export PATH="${DEPS_DIR}/cmake-3.29.0-linux-${CMAKE_ARCH}/bin:$PATH"
  fi
  cmake_ok=true
fi


if [[ -e "${LLVM_DIR}" && -e "${MLIR_DIR}" && -e "${DEPS_DIR}/cudaq/bin" && "$cmake_ok" == true ]]; then
    blank
    ok "All Required Dependencies Installed"
else
    warn "Failed to install all dependencies"
    exit 1
fi

################################## Build the project ###################################

section "Building MQSS-CUDAQ"

ok "Found CUDAQ-Quake path: ${CUDA_QUAKE}"
ok "Found CUDAQ DIR: ${CUDAQ_DIR}"
ok "Found LLVM DIR: ${LLVM_DIR}"
ok "Found MLIR DIR: ${MLIR_DIR}"

# echo "Configuring MQSS Passes Repository CMake..."
# cd  "${BUILD_DIR}"

cmake -G Ninja \
  -S "${CURRENT_DIR}/tools/mqss-cudaq" \
  -B "${CURRENT_DIR}/build/tools/mqss-cudaq" \
  -DBUILD_CUDAQ=ON \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DMLIR_DIR="${MLIR_DIR}"\
  -DLLVM_DIR="${LLVM_DIR}"\
  -DBUILD_MLIR_PASSES_DOCS="${BUILD_DOCS}" \
  -DCUDAQ_SOURCE_DIR="${CUDAQ_DIR}" \
  -DMQSS_SRC_DIR="${CURRENT_DIR}" \
  -DINSTALL_DIR="${INSTALL_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

blank
info "Building MQSS Repository Passes with ${NUM_JOBS} jobs..."
ninja -j"${NUM_JOBS}" -C "${CURRENT_DIR}/build/tools/mqss-cudaq"
# echo "Successfully built MQSS-CUDAQ!"
# echo " "
blank

if [[ -e "${INSTALL_DIR}/bin/mqss-cudaq-opt" ]]; then
    ok "Successfully installed MQSS-CUDAQ"
else
    fail "MQSS-CUDAQ not installed"
    exit 1
fi