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

# Development Guide

Ready to contribute to the Quantum Compilation Suite of the MQSS? This guide will help you get
started.

## Development Environment

- It is recommended to use a docker container to ensure consistent, stable development environment.
- The required `DockerFile` and `devcontainer.json` are provided in `.devcontainer` directory. Build
  and RUN the docker container using the following commands:

```sh
docker build -t mqss-pass-dev -f .devcontainer/Dockerfile .
docker run --rm -it \
  -v "$PWD":/workspaces/MQSS-Quantum-Compilation-Suite \
  -w /workspaces/MQSS-Quantum-Compilation-Suite \
  mqss-pass-dev \
  bash
```

## Building the tool

- The driver is the `Makefile` within the project root.
- The Makefile invokes build scripts within `scripts/`. `build.sh`: Main script for configuring the
  build for all targets including `mqss-opt`

- The ``make` commands to build the targets remain the same as in the [README](../../README.md).
  Ensure the sequence of commands is followed.

## Project structure (for Current Release)

```
MQSS-Quantum-Compilation-Suite/
├── cmake/                      # Find*.cmake modules: CUDAQ, Catalyst, QDMI, MQT-QMAP, MQT-QCEC, etc.
├── docs/                       # Sphinx documentation source (this page included)
├── include/
│   ├── Passes/
│   │   ├── Analysis/
│   │   │   ├── CatalystExtractor.h
│   │   │   ├── DialectAnalysisSelector.h
│   │   │   ├── Extractor.h
│   │   │   └── QuakeExtractor.h
│   │   ├── CodeGen/
│   │   │   ├── BasisConversionPatterns.h
│   │   │   ├── CMakeLists.txt
│   │   │   ├── CodeGenPasses.h
│   │   │   ├── CodeGenPasses.td
│   │   │   ├── Patterns.h
│   │   │   └── StaticAllocas.h
│   │   ├── Transforms/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── Decomposition.h
│   │   │   ├── Dialects.h
│   │   │   ├── MappingPassUtils.h
│   │   │   ├── PassIncludes.h
│   │   │   ├── PassUtils.h
│   │   │   ├── Pipelines.h
│   │   │   ├── Transforms.h
│   │   │   ├── Transforms.td
│   │   │   └── TranspilationPassUtils.h
│   │   └── CMakeLists.txt
│   ├── Utils/
│   │   ├── debug_utils.h
│   │   ├── dialectutils.h
│   │   └── Error.h
│   └── CMakeLists.txt
├── lib/
│   ├── Dialects/
│   │   ├── CMakeLists.txt
│   │   └── Dialects.cpp
│   ├── Passes/
│   │   ├── CodeGen/
│   │   │   ├── BasisConversionPass.cpp
│   │   │   ├── CMakeLists.txt
│   │   │   ├── ConversionPatterns.cpp
│   │   │   ├── GlobalizeArrayValuesPass.cpp
│   │   │   ├── LLVMDialectToLLVMIRPass.cpp
│   │   │   ├── QuakeToQASM2Pass.cpp
│   │   │   ├── QuantumToLLVMDialectPass.cpp
│   │   │   └── StaticAllocas.cpp
│   │   ├── Transforms/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── CommonCNOTReversalPass.cpp
│   │   │   ├── CommonCommuteAndSwitchPass.cpp
│   │   │   ├── CommonDecompositionPass.cpp
│   │   │   ├── CommonGateCancellationPass.cpp
│   │   │   ├── CommonGateCommutationPass.cpp
│   │   │   ├── CommonMappingPass.cpp
│   │   │   ├── CommonNormalizeArgAnglePass.cpp
│   │   │   ├── CommonPatternReductionPass.cpp
│   │   │   └── Pipeline.cpp
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
├── scripts/                    # build.sh, mqss-cc wrapper, front-end toolchain download scripts
├── tests/                      # tests/dialects (lit + FileCheck) and tests/code (end-to-end)
├── CMakeLists.txt
└── mqss-cc.cpp
```

- Dialect registration lives in `lib/Dialects`. Dialect-agnostic MLIR optimization passes (namespace
  `mqss::opt`) live in `lib/Passes/Transforms`. Code-generation/lowering passes (namespace
  `mqss::codegen`) live in `lib/Passes/CodeGen`.

- CUDAQ and Catalyst are included as external dependencies and are downloaded and installed as
  `cmake` modules. Following targets are built for each of these modules:
  - CUDAQ: `QuakeDialect CCDialect QECDialect OptimBuilder OptCodeGen`
  - Catalyst : `MLIRMBQC MLIRQRef MLIRQuantum` These targets incorporate all dialect related headers
    and API implementations.

## CMakeLists.txt

The pass library is built as three separate CMake targets, one per directory under `lib/`, each with
its own `CMakeLists.txt` declaring exactly the external dependencies (CUDAQ, Catalyst, QDMI,
MQT-QMAP, MLIR conversion libraries, etc.) that its own sources actually need:

- `lib/Dialects/CMakeLists.txt` — builds `MQSSSupportedDialects` (dialect registration).
- `lib/Passes/CodeGen/CMakeLists.txt` — builds `MQSSCICodeGenPasses` (code-generation/lowering
  passes), linking `MQSSSupportedDialects` `PUBLIC`.
- `lib/Passes/Transforms/CMakeLists.txt` — builds `MQSSCIPasses` (dialect-agnostic optimization
  passes), linking `MQSSCICodeGenPasses` `PUBLIC`.

Because each dependency is declared `PUBLIC` at the target that actually needs it, everything
propagates transitively up the chain. `root/CMakeLists.txt` only has to link the final executable
against the top of that chain plus MLIR's own driver library:

```cmake
target_link_libraries(mqss-opt PUBLIC MLIROptLib MQSSCIPasses)
```

Note: this transitive propagation is convenient, but it isn't a substitute for checking what a given
`.cpp` file's callees actually require. If a source file (or a library it calls into, such as
`CUDAQ::CodeGen`) needs a specific MLIR conversion library, link it explicitly at the target that
contains that source file — don't assume it will arrive transitively from another target further
down the chain.

## Testing

- We use python-lit along-with ninja and FileCheck to perform dialect-level (input: MLIR dialect;
  output: MLIR dialect) and optionally end-to-end (input:c++/python code; output: MLIR dialect)
  testing. In the end, what is tested for correctness is the output optimized/transformed MLIR
  dialect. In a select few cases, especially the `CodeGen` passes, the backend exchange formats e.g.
  QIR or OpenQasm2 are tested for correctness.

- The tests can be found in directories : `tests/dialects` and `tests/code`.

- Dialect-level testing

  1. The input dialect is annotated with the RUN command, for e.g.:

  `// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-X 2>&1 | FileCheck %s` which runs the
  target/executable `mqss-opt` along-with the pass `CommonCommutePass`. FileCheck looks for strings
  to match, specified using the `CHECK:` keyword, for e.g.

  ```sh
  // CHECK: %out_qubits = quantum.custom "PauliX"() %2 : !quantum.bit
  // CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
  ```

  If an exact match is found in the output dialect, the test succeeds, otherwise the test fails.

  1. End-to-End testing (Optional) In this testing, the input is a c++/python code, which is then
     translated to the input MLIR dialect. The testing then proceeds as in (1). The emphasis here is
     on testing the front-end translation pipeline as well as the transformed dialects. A wrapper
     script `mqss-cc` is used to invoke the necessary tools to translate the c++/python code to the
     input quake or catalyst-quantum dialect. If the input code to MLIR dialect translation fails,
     then the test itself will fail. An example test file performing such a test is shown below:

     ```sh
      // RUN: %mqss-cc %S/../CommuteCNotRxPass.cpp --passes=CommonCommutePass=mode=CX-RX | FileCheck %s

      // CHECK: quake.rx (%cst_1) %4 : (f64, !quake.ref) -> ()
      // CHECK-NEXT: quake.x [%1] %2 : (!quake.ref, !quake.ref) -> ()
     ```

  The test proceeds as follows:

  1. mqss-cc parses the command-line arguments
     `%S/../CommuteCNotRxPass.cpp --passes=CommonCommutePass=mode=CX-RX`
  2. Since, a c++ code is the input, it is assumed that the code contains a quantum circuit defined
     using cudaq (will be updated in the future).
  3. The c++ to quake dialect translation pipeline is invoked via the tool `cudaq-quake`
  4. Once the input dialect is generated, `mqss-opt` tool is used to apply MQSS
     optimization/translation passes defined using the `--passes` flag.
  5. Finally, the optimized/transformed dialect is emitted which is checked by FileCheck.

  Note: Make sure that the path to `cudaq-quake` and `mqss-opt` are appended to the `$PATH`
  environment variable of your shell via the command : `eval "$(make set-target-paths)"`.

## Enabling Pass Debug Information

The Pass debug information can be enabled by passing in the flag `--debug` to the `DEBUG_FLAG`
variable within the Makefile. Simply remove the flag if no debug information is needed.
