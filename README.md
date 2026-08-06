<!--------------------------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
----------------------------------------------------------------------------------------------------->

<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/_static/mqss_logo_dark.svg" width="20%">
    <img src="./docs/_static/mqss_logo.svg" width="20%">
  </picture>
</div>

# Munich Quantum Software Stack (MQSS)-Quantum Compilation Suite

This repository contains a collection of compiler passes integrated into the MQSS to optimize,
transform, and lower quantum programs to instructions compliant with target Quantum devices. The
passes in this suite operate on quantum circuits represented using the state-of-the-art
[Multi-Level Intermediate Representation (MLIR)](https://mlir.llvm.org) framework.

The passes operate on either the quake MLIR dialect by Nvidia
[cudaq-quantum](https://github.com/NVIDIA/cuda-quantum) or catalyst-quantum dialect by
[pennlylane-catalyst](https://github.com/PennyLaneAI/catalyst).

<span style="color: red;">[NOTE] : This Suite is still under active development. You can expect
bugs.</span>

<div align="center">
  <a href="https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/">
  <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
</div>

## Key features

1. **Representation-Agnostic Optimizations**: Apply the same passes across multiple MLIR dialects.

2. **Cross-framework support**: Works with ecosystems like cudaq-quantum and Catalyst.

3. **Write once, reuse everywhere**: Shared optimization logic across compiler representations.

4. **Built to extend**: Add new passes without redesigning the framework.

5. **Native-IR interoperability**: Connect frameworks without replacing their IRs/dialects.

6. **Qubit Mapping**: A representation-agnostic logical/algorithmic to physical qubit mapping pass
   that uses [QDMI](https://github.com/Munich-Quantum-Software-Stack/QDMI) and
   [MQT-QMAP](https://github.com/munich-quantum-toolkit/qmap)

7. **Transpilation**: A transpilation pass to decompose to the native-gate set of target quantum
   devices (currently only for cudaq-quake).

Note: Please refer to the
[FAQs](https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/docs/faqs/index.md)
section for more details on the passes, MLIR, MQSS and other related questions.

## Getting Started

### System Requirements

- OS : Linux (tested on Ubuntu 22.04)
- Architecture : aarch64, X86

### Development Environment

- Docker
- VSCode
- VSCode Dev Containers extension

### Major Dependencies

Note: These are automatically downloaded and installed by the build scripts

- LLVM : 22.1.0 toolchain
- CMake : 3.19...3.30
- cudaq-quantum toolchain : 0.15.0
- pennylane-catalyst toolchain: 0.14.1
- python : 3.11
- C++ : 17...20
- Compiler : gcc and g++ 11.4

For a full list of dependencies check `.devcontainer/Dockerfile`.

### Prerequisites

Clone the project:

```bash
git clone https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite.git \
       /workspaces/MQSS-Quantum-Compilation-Suite
cd /workspaces/MQSS-Quantum-Compilation-Suite
git checkout <branch-name>
```

`branch-name` could be `develop` or any other branch from this repository. If using docker, RUN the
commands:

```sh
docker build -t mqss-pass-dev -f .devcontainer/Dockerfile .
docker run --rm -it \
  -v "$PWD":/workspaces/MQSS-Quantum-Compilation-Suite \
  -w /workspaces/MQSS-Quantum-Compilation-Suite \
  mqss-pass-dev \
  bash
```

Note: The project root is at `/workspaces/MQSS-Quantum-Compilation-Suite`

## Building and Installing the project

First, we need to configure the build via `cmake` by running the command:

```bash
make build
```

This invokes the scripts `scripts/build.sh` which downloads and installs all the required
dependencies for building the target `mqss-opt`. This script contains the required cmake commands to
configure the project.

Finally, build the targets by running:

```bash
make target
```

This builds the targets using the `ninja` build system and if the build succeeds, generates the
executable `mqss-opt`. You can change the installation directory by modifying the `INSTALL_DIR`
variable within the MakeFile.

Next, we need to set paths to the directories where the executables are generated i.e. `build/bin`.
RUN command:

```bash
eval "$(make set-target-paths)"
```

- If you make any changes to the source code i.e. to the C++ files within `lib/*`, then just rerun
  the `make target` command.

- If any changes are made to the build script i.e. `build.sh` or to the CMakeLists or to the files
  within `include/` then do `make build` first and then `make target`.

## Testing the installation

After the build is successful, use the following commands to test the installation.

For mlir dialect-level testing (faster), RUN:

```bash
make test-dialects
```

This command will run all the available test cases in the `tests/dialects` directory. There are a
total of about 30 test cases currently, with more added regularly.

## Contact

The development of this project is led by the QCT department at the LRZ and the QSI department at
MQV gGmbH. You can also always reach us at <mqss@munich-quantum-valley.de>.

Please try to use the publicly accessible GitHub channels (issues, discussions, pull requests) to
allow for a transparent and open discussion as much as possible.
