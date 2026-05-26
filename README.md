<!----------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
---------------------------------------------------------------------------->

<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/_static/mqss_logo_dark.svg" width="20%">
    <img src="./docs/_static/mqss_logo.svg" width="20%">
  </picture>
</div>

# Munich Quantum Software Stack-Compiler Suite (MQSS-CI)

<!-- [DOXYGEN MAIN] -->

This repository contains a collection of compiler passes integrated into the MQSS
to optimize, transform, and lower quantum programs. The passes in this suite operate on
quantum circuits represented using the state-of-the-art [Multi-Level Intermediate Representation
(MLIR)](https://mlir.llvm.org) framework.

The passes operate on either the quake MLIR dialect by Nvidia [cudaq-quantum](https://github.com/NVIDIA/cuda-quantum) or catalyst-quantum dialect by [pennlylane-Catalyst](https://github.com/PennyLaneAI/catalyst).

## Key features

1. __Representation-agnostic optimizations__: Apply the same passes across multiple quantum IRs.

2. __Cross-framework support__: Works with ecosystems like cudaq-quantum and Catalyst.

3. __Write once, reuse everywhere__: Share optimization logic across compiler representations.

4. __Built to extend__: Add new passes without redesigning the framework.

5. __Native-IR interoperability__: Connect frameworks without replacing their IRs.

6. __Qubit Circuit Mapping__: A representation-agnostic logical to physical qubit mapping pass using [MQT-QMAP](https://github.com/munich-quantum-toolkit/qmap)

Note: Please refer to the FAQs section for more details on the passes, MLIR, MQSS and other related questions.
<!-- [DOXYGEN MAIN] -->

<div align="center">
  <a href="https://munich-quantum-software-stack.github.io/MQSS-Passes-Documentation/mlir/">
  <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
</div>

## Getting Started

### System Requirements

- OS : Linux (tested on Ubuntu 22.04)
  - Use docker container if on a different OS
- Architecture : aarch64, X86

### Development Environment

- Docker
- VSCode
- VSCode Dev Containers extension

### Major Dependencies

Note: These are automatically downloaded and installed by the build scripts

- Clang+LLVM : 16.0.6 toolchain and 21.8 toolchain
- CMake : 3.29...4.2
- cudaq-quantum toolchain : 0.14.0
- pennylane-catalyst toolchain: 0.14.1
- python : 3.11
- C++ : 17...20
- Compiler : gcc and g++ 11.4

For a full list of dependencies check ```.devcontainer/Dockerfile```.

### Prerequisites

Clone the project:

```bash
git clone https://github.com/akshay9594/MQSS-Passes-Suite.git \
       /workspaces/MQSS-Passes-Suite
cd /workspaces/MQSS-Passes-Suite
git checkout bfbc0832ecd23de753b23f749f19bcba683af2e2
```

If using docker, RUN the commands:

```sh
docker build -t mqss-pass-dev -f .devcontainer/Dockerfile .
docker run --rm -it \
  -v "$PWD":/workspaces/MQSS-Passes-Suite \
  -w /workspaces/MQSS-Passes-Suite \
  mqss-pass-dev \
  bash
```

Note: The project root is at ```/workspaces/MQSS-Passes-Suite```

## Building and Installing the project

The first thing to do is to setup a virtual env and append the installation
path to the ```PATH``` env variable. The default install path is ```~/.local/bin```
which is where the executables are generated. The MakeFile does this
for you. RUN:

```bash
eval "$(make setup-env)"
```

Then, configure the project by running the command:

```bash
make build
```

This invokes two scripts ```scripts/build_cudaq.sh``` and ```scripts/build_catalyst.sh```.
These scripts contain the required cmake commands to configure the project.

Finally, build the project by running:

```bash
make target
```

This builds the project using ```ninja``` and if the build succeeds,
generates the executables ```mqss-catalyst-opt```, ```mqss-cudaq-opt``` and ```mqss-cc```.
You can change the install path by modifying the ```INSTALL_PATH```
variable within the  MakeFile.

- If you make any changes to the source
code i.e. to the C++ files within ```MQSS-Passes-Suite/lib/*```, then just rerun the ```make target```
command.

- If any changes are made to the build scripts i.e. ```build_catalyst.sh``` or ```build_cudaq.sh``` or
to the CMakeLists then do ```make build``` first and then ```make target```.

## Enabling Pass Debug Information

## Testing the installation

After the build is successful, use the following commands to test
the installation.

For mlir dialect-level testing (faster), RUN:

```bash
make test-dialects
```

For slower end-to-end testing (input: c++/python code, output: optimized mlir-dialect), RUN:

```bash
make test-all
```

This command will run all the available test cases in the ```tests/dialects``` and
```tests/code``` directories. There are a total of 82 test cases currently, with
more added regularly.

## Citation

```bibtex
@INPROCEEDINGS{letrasMQSSCI2025,
  author={Letras, Martín and Echavarria, Jorge and Farooqi, Muhammad Nufail and De Pascale, Marco and Vera, Mario Hernández and Tornow, Nathaniel and Schulz, Laura and Schulz, Martin},
  booktitle={2025 IEEE International Conference on Quantum Computing and Engineering (QCE)},
  title={Towards a Unified Multi-Target Mlir-Based Compiler: A Heterogeneous Compilation Framework for High-Performance and Quantum Computing Integration},
  year={2025},
  volume={02},
  number={},
  pages={28-33},
  keywords={Performance evaluation;Quantum computing;Runtime;High performance computing;Full stack;Graphics processing units;Optimization;Quantum Compilation;Intermediate Representation (IR);MLIR;HPCQC Integration},
  doi={10.1109/QCE65121.2025.10288}
  }
```

## Acknowledgements
