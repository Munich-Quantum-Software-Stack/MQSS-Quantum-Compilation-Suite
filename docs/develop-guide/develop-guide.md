# Development Guide

<!-- IMPORTANT: Keep the line above as the first line. -->
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
-------------------------------------------------------------------------- -->

<!-- This file is a static page and included in the CMakeLists.txt file. -->

Ready to contribute to the Quantum Compilation Suite of the MQSS? This guide will help you get
started.

## Development Environment

- It is recommended to use a docker container to ensure consistent, stable development environment.
- The required ```DockerFile``` and ```devcontainer.json``` are provided in ```.devcontainer```
  directory. Build and RUN the docker container using the following commands:

```sh
docker build -t mqss-pass-dev -f .devcontainer/Dockerfile .
docker run --rm -it \
  -v "$PWD":/workspaces/MQSS-Passes-Suite \
  -w /workspaces/MQSS-Passes-Suite \
  mqss-pass-dev \
  bash
```

## Building the tool

- The driver script is the ```MakeFile``` within the project root.
- The MakeFile invokes build scripts within ```scripts/```.
   ```build_cudaq.sh```: Downloads and installs dependencies and configures build
                        for target ```mqss-cudaq-opt```
   ```build_catalyst.sh```: Downloads and installs dependencies and configures build
                        for target ```mqss-catalyst-opt```
   ```setup-env.sh```: Sets up a virtual environment and installs python3.11 in it.

- The ```make`` commands to build the targets remain the same as in the README.
  Ensure the sequence of commands is followed.

## Project structure

The key directory which contains the source code is ```lib/```. Its structure is:

```
lib/
├── common/
│   ├── include/
│   └─ Passes/
└── mqss-cudaq/
│   ├── CMakeLists.txt
│   ├── cudaq.cpp
│   ├── include/
│   ├── lib/
│   └── cmake/
└── mqss-catalyst/
    ├── CMakeLists.txt
    ├── catalyst.cpp
    ├── include/
    ├── lib/
    └── cmake/
```

- The Dialect Agnostic MLIR optimization passes can be found in ```common/Passes```
  whereas, dialect specific passes can be found in ```mqss-cudaq/lib/MQSSQuakePasses```
  and ```mqss-catalyst/lib/MQSScatalystPasses```. Currently, passes for either ```quake```
  or ```catalyst-quantum``` dialects are added.

- All dialect related files can be found in ```include/IR``` and ```lib/IR```. Of these,
  the important ones are the table-gen files (.td) found in ```include/IR``` , which comprise
  the dialect operation definitions.

## Testing

- We use python-lit along-with ninja and FileCheck to perform dialect-level (input: MLIR dialect; output: MLIR dialect)
as well as end-to-end (input:c++/python code; output: MLIR dialect) testing. In the end, what is tested
for correctness is the output optimized/transformed MLIR dialect.

- The tests can be found in directories : ```tests/dialects``` and ```tests/code```.

- Dialect-level testing
  1. The input dialect is annotated with the RUN command, for e.g.:

    ```// RUN: %mqss-catalyst-opt %s --CommonCommutePass=mode=CX-X 2>&1 | FileCheck %s```
    which runs the target/executable ```mqss-catalyst-opt``` along-with the pass ```CommonCommutePass```.
    FileCheck looks for strings to match, specified using the ```CHECK:``` keyword, for e.g.

    ```sh
    // CHECK: %out_qubits = quantum.custom "PauliX"() %2 : !quantum.bit
    // CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    ```

    If an exact match is found in the output dialect, the test succeeds, otherwise the test fails.
  2. End-to-End testing
     In this testing, the input is a c++/python code, which is then translated to the input
     MLIR dialect. The testing then proceeds as in (1). The emphasis here is on testing the
     front-end translation pipeline as well as the transformed dialects. A wrapper script
     ```mqss-cc``` is used to invoke the necessary tools to translate the c++/python code
     to the input quake or catalyst-quantum dialect. If the input code to MLIR dialect translation fails,
     then the test itself will fail. An example test file performing such a test is shown below:

     ```sh
      // RUN: %mqss-cc %S/../CommuteCNotRxPass.cpp --passes=CommonCommutePass=mode=CX-RX | FileCheck %s

      // CHECK: quake.rx (%cst_1) %4 : (f64, !quake.ref) -> ()
      // CHECK-NEXT: quake.x [%1] %2 : (!quake.ref, !quake.ref) -> ()
     ```

    The test proceeds as follows:
    1. mqss-cc parses the command-line arguments ```%S/../CommuteCNotRxPass.cpp --passes=CommonCommutePass=mode=CX-RX```
    2. Since, a c++ code is the input, it is assumed that the code contains a quantum circuit defined using
        cudaq .
    3. The c++ to quake dialect translation pipeline is invoked via the tool ```cudaq-quake```
    4. Once the input dialect is generated, ```mqss-cudaq-opt``` tool is used to apply MQSS optimization/translation
       passes defined using the ```--passes``` flag.
    5. Finally, the optimized/transformed dialect is emitted which is checked by FileCheck.

## Enabling Pass Debug Information

The Pass debug information can be enabled by passing in the flag ```--debug``` to the ```DEBUG_FLAG```
variable within the MakeFile. Simply remove the flag if no debug information is needed.

## Enabling clangd for linting

RUN command : ```make compile_commands```.

- This creates a symlink from ```_deps/mqss-cudaq/clang+llvm-16.0.4-aarch64-linux-gnu/bin/clangd```
      to ```/usr/local/bin/clangd```
- It also creates a file ```compile_commands.json``` within ```build/```
