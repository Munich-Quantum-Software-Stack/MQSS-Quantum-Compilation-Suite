# [Operating System]
ARG base_image=ubuntu:22.04

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


ARG CMAKE_ARCH=aarch64
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget ca-certificates && \
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
      liblzma-dev

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libz3-dev \
    git \
    make \
    cmake \
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
RUN git config --global gc.auto 0