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

#### 2. Fetch the correct cuda_quantum installer and set path to CUDA_QUAKE

mkdir -p "${DEPS_DIR}/cudaq"
CUDAQ_DIR="${DEPS_DIR}/cudaq"
CUDAQ_VERSION="0.15.0"
# Note: $CUDAQ_DIR is passed as an input argument to cmake (at the bottom) when building the project
#       $CUDAQ_DIR should already contain the all the required header files for the project (e.g. cudaq.h)

CUDA_QUAKE="${DEPS_DIR}/cudaq/bin/cudaq-quake"
# Fetch OS specific cuda-quantum installer
if [[ -e "$CUDA_QUAKE" ]]; then
  ok "Found CUDAQ ${CUDAQ_VERSION} assets at: ${DEPS_DIR}/cudaq/bin"
else
  warn "CUDAQ Binaries NOT FOUND!"
  if [[ -e "$DEPS_DIR/install_cuda_quantum_cu13.$(uname -m)" ]]; then
    info "Installing CUDAQ ${CUDAQ_VERSION} Assets..."
    bash $DEPS_DIR/install_cuda_quantum*.$(uname -m) --accept -- --installpath "${DEPS_DIR}/cudaq" > /dev/null
  else
    info "Downloading and Installing CUDAQ ${CUDAQ_VERSION} Assets..."
    wget --quiet -P $DEPS_DIR "https://github.com/NVIDIA/cuda-quantum/releases/download/${CUDAQ_VERSION}/install_cuda_quantum_cu13.$(uname -m)"
    bash $DEPS_DIR/install_cuda_quantum*.$(uname -m) --accept -- --installpath "${DEPS_DIR}/cudaq" > /dev/null
  fi

fi

### 3. Install Catalyst

# Fetch OS specific catalyst installer. This is needed to get the catalyst python jit framework
VENV_DIR="${DEPS_DIR}/.venv"
CATALYST_SITE_PACKAGE="${VENV_DIR}/lib/python3.11/site-packages/catalyst"

PENNYLANE_VERSION="0.44.1"
PENNYLANE_CATALYST_VER="0.14.1"
PENNYLANE_LIGHTNING_VERSION="0.45.0"

if [[ -e "${CATALYST_SITE_PACKAGE}" ]]; then
    info "Found Pennylane Catalyst ${PENNYLANE_VERSION} package at: ${CATALYST_SITE_PACKAGE}"
else
    info "Installing Pennylane Catalyst ${PENNYLANE_VERSION}..."
    python3.11 -m pip install -q pennylane==${PENNYLANE_VERSION} pennylane-catalyst==${PENNYLANE_CATALYST_VER} pennylane-lightning==${PENNYLANE_LIGHTNING_VERSION}

fi
