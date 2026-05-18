# [Operating System]
ARG base_image=ubuntu:22.04

# [CUDA-Q Dependencies]
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

## [Environment Variables]
# ENV CUDAQ_INSTALL_PREFIX=/usr/local/cudaq
# ENV CUQUANTUM_INSTALL_PREFIX=/usr/local/cuquantum
# ENV CUTENSOR_INSTALL_PREFIX=/usr/local/cutensor
# ENV LLVM_INSTALL_PREFIX=/usr/local/llvm
# ENV BLAS_INSTALL_PREFIX=/usr/local/blas
# ENV ZLIB_INSTALL_PREFIX=/usr/local/zlib
# ENV OPENSSL_INSTALL_PREFIX=/usr/local/openssl
# ENV CURL_INSTALL_PREFIX=/usr/local/curl
# ENV AWS_INSTALL_PREFIX=/usr/local/aws

# ## [Build Dependencies]
# RUN apt-get update && apt-get install -y --no-install-recommends \
#         wget git unzip \
#         python3-dev python3-pip && \
#     python3 -m pip install --no-cache-dir numpy && \
#     apt-get autoremove -y --purge && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libz3-dev \
    openssh-client \
    libgtest-dev \
    pkg-config \
    libeigen3-dev \
    libboost-program-options-dev \
    libzip-dev && \
    # Clean up cache to reduce image size
    rm -rf /var/lib/apt/lists/*

RUN git config --global gc.auto 0
# RUN git clone https://github.com/NVIDIA/cuda-quantum.git /workspaces/cuda-quantum
RUN make all -C /workspaces/MQSS-Passes-Suite
#RUN rm -rf /root/.llvm-project
