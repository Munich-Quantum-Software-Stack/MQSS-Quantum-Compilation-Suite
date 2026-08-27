# [Operating System]
ARG base_image=ubuntu:22.04

# Prebuilt LLVM/CUDA-Q/Catalyst image (see .devcontainer/deps.DockerFile and
# .github/workflows/deps-image.yml) -- avoids re-fetching and rebuilding these
# from source on every container build. Tag documents the exact pins it was
# built from; override with --build-arg DEPS_IMAGE=... to test a different one,
# and bump the default here whenever cmake/FetchLLVM.cmake, cmake/FindCUDAQ.cmake,
# or cmake/FindCatalyst.cmake change their pinned versions.
#
# Everything inside the image (the LLVM/MLIR toolchain, the compiled CUDA-Q and
# Catalyst archives) is native machine code, so it is published once per
# architecture as <DEPS_IMAGE>-amd64 / <DEPS_IMAGE>-arm64. BuildKit sets
# TARGETARCH ("amd64" or "arm64") to the platform the dev container is being
# built for; the matching stage below is selected and the other one is never
# pulled. An override passed via --build-arg DEPS_IMAGE=... must likewise have
# -amd64 / -arm64 tags published.
ARG DEPS_IMAGE=ghcr.io/akshay9594/mqss-passes-suite/mqssci-deps:llvm22.1.0-cudaq0.15.0-catalyst0.15.0

FROM ${DEPS_IMAGE}-amd64 AS deps-amd64
FROM ${DEPS_IMAGE}-arm64 AS deps-arm64
FROM deps-${TARGETARCH} AS deps

FROM ${base_image} AS prereqs
SHELL ["/bin/bash", "-c"]
ARG toolchain=gcc11

# When a dialogue box would be needed during install, assume default configurations.
# Set here to avoid setting it for all install commands.
# Given as arg to make sure that this value is only set during build but not in the launched container.
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && \
    apt-get autoremove -y --purge && apt-get clean && rm -rf /var/lib/apt/lists/*

## [Prerequisites]
# RUN apt-get update && apt-get install -y --no-install-recommends python3 && \
#     apt-get autoremove -y --purge && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y locales && \
    sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && \
    locale-gen
ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget ca-certificates

ARG TARGETARCH

RUN case "$TARGETARCH" in \
      amd64) CMAKE_ARCH="x86_64" ;; \
      arm64) CMAKE_ARCH="aarch64" ;; \
      *) echo "Unsupported TARGETARCH: $TARGETARCH" >&2; exit 1 ;; \
    esac  && \
    wget https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz && \
    tar -xzf cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz -C /opt && \
    ln -sf /opt/cmake-3.29.0-linux-${CMAKE_ARCH}/bin/cmake /usr/local/bin/cmake && \
    ln -sf /opt/cmake-3.29.0-linux-${CMAKE_ARCH}/bin/ctest /usr/local/bin/ctest && \
    rm cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      curl \
      ca-certificates \
      libssl-dev \
      zlib1g-dev \
      libbz2-dev \
      libreadline-dev \
      libsqlite3-dev \
      libncursesw5-dev \
      xz-utils \
      tk-dev \
      libxml2-dev \
      libxmlsec1-dev \
      libffi-dev \
      libzstd-dev \
      zstd \
      clangd \
      liblzma-dev

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libz3-dev \
    git \
    make \
    libtinfo-dev \
    lld \
    jq \
    gcc-11 \
    g++-11 \
    python3 \
    python3-pip \
    ninja-build \
    openssh-client \
    openssl \
    pkg-config \
    libeigen3-dev \
    libboost-program-options-dev \
    nano \
    vim \
    libzip-dev && \
    # Clean up cache to reduce image size
    rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install lit

RUN apt-get update && apt-get install -y --no-install-recommends \
    locales
RUN python3 -m pip install sphinx myst-parser furo

RUN git config --global gc.auto 0

# ---- Prebuilt heavy dependencies (LLVM/MLIR, CUDA-Q, Catalyst) --------------
# Pulled in from the deps stage above instead of each being fetched+built from
# source at CMake configure time. FindCUDAQ.cmake/FindCatalyst.cmake/
# FetchLLVM.cmake all check these locations (via env var / CMAKE_PREFIX_PATH)
# before falling back to their own auto-fetch logic, so this is a pure
# accelerant -- deleting this block still works, just slower.
COPY --from=deps /opt/deps /opt/deps
ENV LLVM_DIR=/opt/deps/llvm/lib/cmake/llvm \
    MLIR_DIR=/opt/deps/llvm/lib/cmake/mlir \
    CUDAQ_INSTALL_PREFIX=/opt/deps/cudaq \
    CATALYST_INSTALL_PREFIX=/opt/deps/catalyst
