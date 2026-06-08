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

# Getting Started

## System Requirements

- OS : Linux (tested on Ubuntu 22.04)
  - Use docker container if on a different OS
- Architecture : aarch64, X86

## Development Environment

- Docker
- VSCode
- VSCode Dev Containers extension

## Dependencies

- A list of major packages/toolchains required is mentioned in [dependencies](dependencies.md)
- For packages required for the development environment, please refer to the
  ```DockerFile```.

## Prerequisites

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

The first thing to do is to setup a virtual environment and install
python3.11 into it. We also need to install cmake. RUN:

```bash
make setup-env
```

Next, we need to set paths to the directories where the executables are generated
i.e. ```~/.local/bin``` as well as path to ```cudaq-opt```. RUN command:

```bash
eval "$(make set-target-paths)"
```

Then, configure the build by running the command:

```bash
make build
```

This invokes two scripts ```scripts/build_cudaq.sh``` and ```scripts/build_catalyst.sh```.
These scripts download and install all the required dependencies for building the targets
(```mqss-cudaq-opt``` and ```mqss-catalyst-opt```). These scripts contain the required
cmake commands to configure the project.

Finally, build the targets by running:

```bash
make target
```

This builds the targets using ```ninja``` and if the build succeeds,
generates the executables ```mqss-catalyst-opt```, ```mqss-cudaq-opt```.
You can change the installation directory by modifying the ```INSTALL_DIR```
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
