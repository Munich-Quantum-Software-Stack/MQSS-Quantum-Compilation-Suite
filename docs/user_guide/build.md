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

# Getting Started

Note: It is highly recommended to use docker container to build and install the project.

## System Requirements

- OS : Linux (tested on Ubuntu 22.04)
- Architecture : aarch64, X86

## Development Environment

- Docker
- VSCode
- VSCode Dev Containers extension

## Dependencies

- A list of major packages/toolchains required is mentioned in [dependencies](dependencies.md)
- For packages required for the development environment, please refer to the `DockerFile`.

## Prerequisites

Clone the project:

```bash
git clone https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite.git \
       /workspaces/MQSS-Quantum-Compilation-Suite
cd /workspaces/MQSS-Quantum-Compilation-Suite
git checkout <branch-name or commit hash>
```

If using docker, RUN the commands:

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

The steps to configure the project and build the targets remain the same as in the
[README](../../README.md).

## Enabling Pass Debug Information

To enable pass debug information, set the following flag within `Makefile`:

```Makefile
DEBUG_FLAG =  # --debug (if you want pass debug info)
```

By default the debug information is enabled i.e. `DEBUG_FLAG =  --debug`. To disable the debug info,
just remove the `---debug` value.

## Testing the installation

After the build is successful, use the following commands to test the installation.

For mlir dialect-level testing (faster), RUN:

```bash
make test-dialects
```

This command will run all the available test cases in the `tests/dialects` directory. There are a
total of about 30 test cases currently, with more added regularly.

## Source-level Compilation

If C++/Python i.e. Source level compilation and testing is intended follow the following steps:

### Downloading front-end dependencies

Run the following command

```bash
make frontend
```

This will setup a python virtual environment, after which, the script `download_toolchains.sh` is
executed. This script downloads and installs: `CUDA-Q` and `Catalyst` toolchains. We need
`cudaq-quake` (translates C++ to Quake) and the `Catalyst-qjit` (translates python to
catalyst-quantum) tools. After the installation is complete, RUN the COMMAND:

```bash
eval "$(make front-end-paths)"
```

This will activate the virtual environment and add `cudaq-quake` to the `$PATH` environment
variable. Check using:

```bash
which cudaq-quake
```

### Testing the Front-end Compilation pipeline

The testing for the frontend involves the script `mqss-cc`. Check that is also on the `$PATH`
environment variable. After this, RUN the command:

```bash
make test-all
```

This runs the test-cases within `tests/code`.
