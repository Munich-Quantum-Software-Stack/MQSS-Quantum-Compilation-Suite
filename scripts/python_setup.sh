#!/usr/bin/env bash
set -euo pipefail

REQUIRED_VERSION="3.11.14"

CURRENT_DIR="$(pwd)"
DEPS_DIR="${CURRENT_DIR}/_deps/mqss-catalyst"
VENV_DIR="${DEPS_DIR}/venv"

mkdir -p "${DEPS_DIR}"

echo "Using deps dir: ${DEPS_DIR}"

# ─── 1. Ensure pyenv is installed ────────────────────────────────────────────

export PYENV_ROOT="${HOME}/.pyenv"
export PATH="${PYENV_ROOT}/bin:${PYENV_ROOT}/shims:${PATH}"

if ! command -v pyenv >/dev/null 2>&1; then
    echo "pyenv not found. Installing pyenv..."

    if ! command -v git >/dev/null 2>&1; then
        echo "ERROR: git is required to install pyenv."
        echo "Install it first, e.g.: sudo apt install git"
        exit 1
    fi

    git clone https://github.com/pyenv/pyenv.git "${PYENV_ROOT}"

    echo "pyenv installed at ${PYENV_ROOT}"
fi

# Initialize pyenv in this script
eval "$(pyenv init -)"

echo "pyenv path: $(command -v pyenv)"

# ─── 2. Check/install Python 3.11.14 via pyenv ───────────────────────────────

if ! pyenv versions --bare | grep -qx "${REQUIRED_VERSION}"; then
    echo "Python ${REQUIRED_VERSION} not found in pyenv."

    echo "Installing build dependencies may be required, e.g. on Ubuntu/Debian:"
    echo "  sudo apt install -y build-essential libssl-dev zlib1g-dev \\"
    echo "      libbz2-dev libreadline-dev libsqlite3-dev curl libncursesw5-dev \\"
    echo "      xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev"

    echo "Installing Python ${REQUIRED_VERSION}..."
    pyenv install "${REQUIRED_VERSION}"
else
    echo "Python ${REQUIRED_VERSION} already installed via pyenv."
fi

PYTHON_BIN="$(pyenv root)/versions/${REQUIRED_VERSION}/bin/python3.11"

if [ ! -x "${PYTHON_BIN}" ]; then
    echo "ERROR: Expected Python binary not found: ${PYTHON_BIN}"
    exit 1
fi

echo "Using Python: ${PYTHON_BIN}"
"${PYTHON_BIN}" --version

# ─── 3. Create virtual environment ───────────────────────────────────────────

if [ -d "${VENV_DIR}" ]; then
    echo "Virtual environment already exists: ${VENV_DIR}"
else
    echo "Creating virtual environment at ${VENV_DIR}..."
    "${PYTHON_BIN}" -m venv "${VENV_DIR}"
fi

# ─── 4. Verify Python inside venv ────────────────────────────────────────────

VENV_PYTHON="${VENV_DIR}/bin/python"

if [ ! -x "${VENV_PYTHON}" ]; then
    echo "ERROR: venv Python not found: ${VENV_PYTHON}"
    exit 1
fi

INSTALLED_VERSION="$("${VENV_PYTHON}" -c 'import sys; print(".".join(map(str, sys.version_info[:3])))')"

echo "Python inside venv: ${INSTALLED_VERSION}"

if [ "${INSTALLED_VERSION}" != "${REQUIRED_VERSION}" ]; then
    echo "ERROR: Expected Python ${REQUIRED_VERSION}, got ${INSTALLED_VERSION}"
    exit 1
fi


echo "Done. Activate with:"
echo "  source ${VENV_DIR}/bin/activate"