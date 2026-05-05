
#!/bin/bash

REQUIRED_VERSION="3.11.9"   # Specify exact version for pyenv
CURRENT_DIR=$(pwd)

BUILD_DIR=${CURRENT_DIR}"/build"
DEPS_DIR=$BUILD_DIR"/_deps"
VENV_DIR="${DEPS_DIR}/venv"

mkdir -p "${BUILD_DIR}"
mkdir -p "${DEPS_DIR}"

# ─── 1. Install pyenv if missing ─────────────────────────────────────────────
if ! command -v pyenv &>/dev/null; then
    echo "⚠️  pyenv not found. Installing..."
    brew install pyenv

    # Add pyenv init to shell profile
    SHELL_PROFILE="$HOME/.zshrc"   # Change to ~/.bash_profile if using bash
    echo '' >> "$SHELL_PROFILE"
    echo '# pyenv setup' >> "$SHELL_PROFILE"
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> "$SHELL_PROFILE"
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> "$SHELL_PROFILE"
    echo 'eval "$(pyenv init -)"' >> "$SHELL_PROFILE"

    # Load pyenv into the CURRENT session without restarting terminal
    export PYENV_ROOT="$HOME/.pyenv"
    export PATH="$PYENV_ROOT/bin:$PATH"
    eval "$(pyenv init -)"

    echo "✅ pyenv installed."
fi

export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PYENV_ROOT/shims:$PATH"
eval "$(pyenv init -)"
echo "🔍 pyenv python path: $(which python)"

# ─── 3. Install Python 3.11 via pyenv (isolated, not system-wide) ────────────
if ! pyenv versions --bare | grep -q "^$REQUIRED_VERSION$"; then
    echo "⚠️  Python $REQUIRED_VERSION not found in pyenv. Installing..."
    pyenv install "$REQUIRED_VERSION"
    echo "✅ Python $REQUIRED_VERSION installed via pyenv."
else
    echo "✅ Python $REQUIRED_VERSION already available in pyenv."
fi

# ─── 4. Set the version locally for this project ─────────────────────────────
pyenv local "$REQUIRED_VERSION"   # Creates a .python-version file in current dir
echo "✅ Python $REQUIRED_VERSION set as local version."

# ─── 5. Create virtual environment ───────────────────────────────────────────
if [ -d "$VENV_DIR" ]; then
    echo "📁 '$VENV_DIR' already exists. Skipping creation."
else
    echo "🔧 Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
    echo "✅ Virtual environment created in '$VENV_DIR'."
fi

