#!/usr/bin/env bash

############# Pretty-print variables#################

blank() {
  printf '\n'
}

info() {
  printf '🔧 %s\n' "$1"
}

ok() {
  printf '✅ %s\n' "$1"
}

########################## Create build and deps directories #############################

CURRENT_DIR=$(pwd)
BUILD_DIR=${CURRENT_DIR}"/build"

# Create directories if they don't exist
mkdir -p "${BUILD_DIR}"

CC="gcc"
CXX="g++"

# Default values
NUM_JOBS=4  # Default number of jobs
BUILD_TYPE="Release"  # Default: Release mode
INSTALL_DIR="${BUILD_DIR}/bin"

# Parse command-line arguments
# TODO: Worth adding a BUILD_TESTS variable to enable/disable
#       building tests. Not needed now since test cases do not
#       need to be built.
while [[ $# -gt 0 ]]; do
  case $1 in
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done


################################## Configure the project ###################################

info "Configuring the MQSS-Quantum-Compilation Suite"

cmake -G Ninja \
  -B "${BUILD_DIR}" \
  -DCUDAQ_AUTO_FETCH=ON \
  -DCATALYST_AUTO_FETCH=ON \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DINSTALL_DIR="${INSTALL_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_INTERFACES_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ok "Configured Build, PLEASE RUN: make target"
