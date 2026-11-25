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
docker build -t mqss-ci-dev -f .devcontainer/Dockerfile .
docker run --rm -it \
  -v "$PWD":/workspaces/MQSS-Quantum-Compilation-Suite \
  -w /workspaces/MQSS-Quantum-Compilation-Suite \
  mqss-ci-dev \
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
├── cmake/                      # Find*.cmake modules: CUDAQ, Catalyst, QDMI, MQT-Core, MQT-QMAP, MQT-QCEC, etc.
├── docs/                       # Sphinx documentation source (this page included)
├── include/
│   ├── MQSSCIInterfaces/
│   │   └── MQSSCompiler.h      # Simplified C++ library API (see Integrate guide)
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
│   │   ├── Pipelines/
│   │   │   └── Pipelines.h     # O1/O2/O3 and the QASM/QIR output pipelines
│   │   ├── Transforms/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── Dialects.h
│   │   │   ├── PassLogic.h
│   │   │   ├── TransformPasses.h
│   │   │   └── TransformPasses.td
│   │   ├── Verification/
│   │   │   └── Instrumentation.h
│   │   └── CMakeLists.txt
│   ├── Translation/
│   │   ├── MLIRToTarget.h      # MLIR -> OpenQASM / IQM JSON / QIR / LLVM IR translations
│   │   └── QASM2ToQuake.h      # OpenQASM 2 -> Quake translation
│   ├── Utils/
│   │   ├── BasisConversionUtils.h
│   │   ├── CodegenUtils.h
│   │   ├── DebugUtils.h
│   │   ├── DialectUtils.h
│   │   ├── Error.h
│   │   ├── MQTCoreUtils.h      # MLIR -> qc::QuantumComputation (shared by QASM output and verification)
│   │   ├── QDMI_Utils.h
│   │   └── TransformUtils.h
│   └── CMakeLists.txt
├── lib/
│   ├── Dialects/
│   │   ├── CMakeLists.txt
│   │   └── Dialects.cpp
│   ├── MQSSCIInterfaces/
│   │   ├── CMakeLists.txt
│   │   └── MQSSCompiler.cpp
│   ├── Passes/
│   │   ├── CodeGen/
│   │   │   ├── BasisConversionPass.cpp
│   │   │   ├── CMakeLists.txt
│   │   │   ├── ConversionPatterns.cpp
│   │   │   ├── GlobalizeArrayValuesPass.cpp
│   │   │   ├── LLVMDialectToLLVMIRPass.cpp
│   │   │   ├── MLIRToQASMPass.cpp
│   │   │   ├── QuakeToQASM2Pass.cpp
│   │   │   ├── QuantumToLLVMDialectPass.cpp
│   │   │   ├── StaticAllocas.cpp
│   │   │   └── TranslateToIQMJSON.cpp
│   │   ├── Pipelines/
│   │   │   ├── CMakeLists.txt
│   │   │   └── Pipeline.cpp
│   │   ├── Transforms/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── CommonCNOTReversalPass.cpp
│   │   │   ├── CommonCommuteAndSwitchPass.cpp
│   │   │   ├── CommonGateCancellationPass.cpp
│   │   │   ├── CommonGateCommutationPass.cpp
│   │   │   ├── CommonMappingPass.cpp
│   │   │   ├── CommonNormalizeArgAnglePass.cpp
│   │   │   ├── CommonPatternReductionPass.cpp
│   │   │   └── QuakeExpandMeasurements.cpp
│   │   ├── Verification/
│   │   │   ├── CMakeLists.txt
│   │   │   └── EquivalenceVerification.cpp
│   │   └── CMakeLists.txt
│   ├── Translation/
│   │   ├── CMakeLists.txt
│   │   ├── MLIRToTarget.cpp
│   │   └── QASM2ToQuake.cpp
│   └── CMakeLists.txt
├── scripts/                    # build.sh, mqss-cc wrapper, front-end toolchain download scripts
├── tests/                      # tests/dialects (lit + FileCheck), tests/unittests (GoogleTest), tests/code (reference)
├── CMakeLists.txt
├── mqss-cc.cpp                 # the mqss-opt tool
└── mqss-translate.cpp          # the mqss-translate tool
```

- Dialect registration lives in `lib/Dialects`. Dialect-agnostic MLIR optimization passes (namespace
  `mqss::mqssci::opt`) live in `lib/Passes/Transforms`. Code-generation/lowering passes (namespace
  `mqss::mqssci::codegen`) live in `lib/Passes/CodeGen`. The preset pipelines (`O1`/`O2`/`O3`) and
  the pipelines that build the output stage (`QASMLoweringPipeline`, `QIRConversionPipeline`) live
  in `lib/Passes/Pipelines`. Circuit-equivalence verification (namespace `mqss::mqssci::verify`)
  lives in `lib/Passes/Verification` — see [Verification Infrastructure](verification.md).
- `lib/Translation` holds the `mqss-translate` translations (namespace `mqss::mqssci::translation`):
  the OpenQASM 2 importer (`QASM2ToQuake.cpp`) and the exporters (`MLIRToTarget.cpp`), which run a
  pipeline from `lib/Passes/Pipelines` and stream its text output. See
  [Translating Circuits with mqss-translate](../user_guide/mqss-translate.md).
- `lib/MQSSCIInterfaces` wraps the pass library behind a single-call `MQSSCompiler` API for
  consumers who don't need direct `mlir::OpPassManager` access — see
  [Integrating the MQSS Quantum Compilation Suite](integrate.md).

- CUDAQ and Catalyst are included as external dependencies and are downloaded and installed as
  `cmake` modules. Following targets are built for each of these modules:
  - CUDAQ: `QuakeDialect CCDialect QECDialect OptimBuilder OptCodeGen`
  - Catalyst : `MLIRMBQC MLIRQRef MLIRQuantum` These targets incorporate all dialect related headers
    and API implementations.
  - MQT-Core, MQT-QMAP, and MQT-QCEC are fetched similarly and are what the verification
    infrastructure and the OpenQASM output use to represent circuits and run equivalence checks.

## CMakeLists.txt

The pass library is built as one CMake target per directory under `lib/`, each with its own
`CMakeLists.txt` declaring exactly the external dependencies (CUDAQ, Catalyst, QDMI, MQT-Core,
MQT-QMAP, MQT-QCEC, MLIR conversion libraries, etc.) that its own sources actually need:

- `lib/Dialects/CMakeLists.txt` — builds `MQSSSupportedDialects` (dialect registration).
- `lib/Passes/CodeGen/CMakeLists.txt` — builds `MQSSCICodeGenPasses` (code-generation/lowering
  passes), linking `MQSSSupportedDialects` `PUBLIC`. This is the only `SHARED` library.
- `lib/Passes/Transforms/CMakeLists.txt` — builds `MQSSCITransformPasses` (dialect-agnostic
  optimization passes), linking `MQSSSupportedDialects`, QDMI and MQT `PUBLIC`. It does not depend
  on the code-generation library.
- `lib/Passes/Verification/CMakeLists.txt` — builds `MQSSCIVerificationPasses` (equivalence-checking
  instrumentation, see [Verification Infrastructure](verification.md)), linking
  `MQSSSupportedDialects`, `MQT::CoreIR`, and `MQT::QCEC` `PUBLIC`.
- `lib/Passes/Pipelines/CMakeLists.txt` — builds `MQSSCIPipelines` (the preset and output
  pipelines), linking `MQSSCICodeGenPasses` and `MQSSCITransformPasses` `PUBLIC`, since a pipeline
  is built out of passes from both.
- `lib/Translation/CMakeLists.txt` — builds `MQSSCITranslation` (the `mqss-translate` translations),
  linking `MQSSCIPipelines`, `MQSSCICodeGenPasses` and `MQSSCITransformPasses` `PUBLIC`.
- `lib/MQSSCIInterfaces/CMakeLists.txt` — builds `MQSSCIInterfaces` (the `MQSSCompiler` library
  API), linking the transform, verification, pipeline, translation and code-generation libraries
  `PUBLIC`. Exposed to external consumers as the `mqss-ci::mqss-ci` alias (see
  [Integrate](integrate.md)).

Note: apart from `MQSSCICodeGenPasses`, the libraries are built as plain static libraries (no
`SHARED` keyword). `add_mlir_library` builds a static library in two pieces — an `obj.<name>` object
library that does the actual compiling, and a `<name>` static archive that just packages its
already- compiled objects — so `target_compile_options`/`target_compile_definitions` calls must
target `obj.<name>` (guarded by `if(TARGET obj.<name>)`) to actually affect compilation; targeting
`<name>` alone silently does nothing. See
[Verification Infrastructure](verification.md#a-cmake-pitfall-specific-to-this-library) for a worked
example of this pitfall. Relatedly, the pass libraries used to be built `SHARED`; they were switched
to static because two `SHARED` libraries that both statically link overlapping LLVM component
libraries (as these do, transitively through `MLIRPass`/`MLIRTransforms`) each end up with their own
private copy of LLVM's global command-line option registry, which crashes at runtime with an "option
registered more than once" error the moment both are loaded into the same process.

Because each dependency is declared `PUBLIC` at the target that actually needs it, everything
propagates transitively up the chain. `root/CMakeLists.txt` only has to link each executable against
the top of its chain plus MLIR's own driver library:

```cmake
target_link_libraries(mqss-opt PUBLIC MLIROptLib MQSSCITransformPasses MQSSCIPipelines
                                      MQSSCIVerificationPasses MQSSCICodeGenPasses)
target_link_libraries(mqss-translate PUBLIC MQSSCITranslation MLIRTranslateLib)
```

Note: this transitive propagation is convenient, but it isn't a substitute for checking what a given
`.cpp` file's callees actually require. If a source file (or a library it calls into) needs a
specific library, declare it explicitly at the target that contains that source file — don't assume
it will arrive transitively from another target. Two cases in this repository are worth knowing:

- **Static archive order.** The linker only scans a static archive once, so a library must come
  _after_ everything that references it. CUDA-Q's `libOptCodeGen.a` uses the Quake and QEC dialects
  and MLIR's Complex/Math/Func-to-LLVM conversions without declaring them, so
  `cmake/FindCUDAQ.cmake` lists them as the `INTERFACE_LINK_LIBRARIES` of `CUDAQ::CodeGen`. Without
  that, CMake may place them before `libOptCodeGen.a` and the link fails with undefined references
  such as `mlir::populateMathToLLVMConversionPatterns`.
- **LTO is off.** MQT-Core turns on link-time optimization (`ENABLE_IPO`) for Release builds and the
  setting leaks into the whole project. With LTO, the link of the interface test executable dropped
  a `llvm::DenseMap<mlir::TypeID, unsigned>` instantiation that a native MLIR library still
  referenced. `root/CMakeLists.txt` therefore forces `ENABLE_IPO` and
  `CMAKE_INTERPROCEDURAL_OPTIMIZATION` to `OFF` before `find_package(mqt-core)`.

## Testing

- We use python-lit along-with ninja and FileCheck to perform dialect-level testing (input: MLIR
  dialect), and GoogleTest to test the C++ interface. In most lit tests what is checked is the
  output optimized/transformed MLIR dialect. In the tests of the `CodeGen` passes, the backend
  exchange formats (OpenQASM, QIR, IQM JSON, LLVM IR) are checked instead, by piping `mqss-opt` into
  `mqss-translate`.

- The tests can be found in directories : `tests/dialects` (lit), `tests/unittests` (GoogleTest) and
  `tests/code` (end-to-end examples, kept for reference).

- Dialect-level testing

  1. The input dialect is annotated with the RUN command, for e.g.:

  `// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-X 2>&1 | FileCheck %s` which runs the
  target/executable `mqss-opt` along-with the pass `CommonCommutePass`. FileCheck looks for strings
  to match, specified using the `CHECK:` keyword. To test a text format, pipe into `mqss-translate`:

  `// RUN: %mqss-opt %s --BasisConversionPass=gates=prx,cz --cse --canonicalize | %mqss-translate --mlir-to-qasm3 | FileCheck %s`

  Rejections are tested by expecting the command to fail and matching the diagnostic:

  `// RUN: %not %mqss-translate --mlir-to-qasm2 %s 2>&1 | FileCheck %s`

  If every `CHECK:` pattern is found in order in the output, the test succeeds, otherwise the test
  fails.

  1. Write SSA names as FileCheck variables, not literals. MLIR and LLVM renumber values whenever a
     pass changes the IR (`%out_qubits_3`, `%7`), so a literal name breaks the test for no real
     reason. Capture a name where the data flow matters and reuse it, and use a wildcard where it
     does not:

     ```sh
     // CHECK: %[[X:[A-Za-z0-9_]+]] = quantum.custom "PauliX"() %{{[A-Za-z0-9_]+}} : !quantum.bit
     // CHECK: %{{[A-Za-z0-9_]+}}:2 = quantum.custom "CNOT"() %{{[A-Za-z0-9_]+}}, %[[X]] : !quantum.bit, !quantum.bit
     ```

     `%[[X:...]]` defines a variable and `%[[X]]` requires the same value again, so the second line
     only matches a CNOT that consumes the PauliX output.

  2. Watch for operations that appear more than once. A `CHECK:` line matches the _first_ line that
     fits, and FileCheck does not backtrack, so a captured variable can bind to an earlier,
     unintended operation. Consume the earlier operations with a plain `CHECK:` line first, or use
     `CHECK-NEXT:` when two lines are adjacent. A kernel that repeats `%1 = quake.extract_ref`
     before the interesting gate is the usual example. Do not reuse one capture across separate
     functions: SSA names restart in each function, so a shared variable only matches by accident.

  3. End-to-End testing (reference only) The tests in `tests/code` take c++/python code, translate
     it with the wrapper script `mqss-cc` (which invokes `cudaq-quake` or the Catalyst tools and
     then `mqss-opt`) and check the result as in (1). They need the front-end toolchains, are not
     part of the regular test runs, and are kept as reference examples of the end-to-end flow:

     ```sh
      // RUN: %mqss-cc %S/../CommuteCNotRxPass.cpp --passes=CommonCommutePass=mode=CX-RX | FileCheck %s
     ```

     Make sure the paths to `cudaq-quake` and `mqss-opt` are appended to the `$PATH` environment
     variable of your shell via the command : `eval "$(make set-target-paths)"`.

- Interface testing

  `MQSSCompiler` has a GoogleTest suite in `tests/unittests/MQSSCIInterfaces/test_Interface.cpp`,
  with fixtures in `tests/unittests/input` (Quake, Catalyst-quantum and OpenQASM 2 circuits). It
  compiles them to OpenQASM 2/3, QIR and IQM JSON and checks the output and the rejected inputs. Run
  it with `make test-interfaces`; see [Integrate](integrate.md#testing-the-interface).

## Verification

Circuit-equivalence verification is implemented as an `mlir::PassInstrumentation` rather than an
ordinary pass — see [Verification Infrastructure](verification.md) for the implementation details,
including a CMake pitfall specific to its static library target that's easy to repeat when adding
new compile options elsewhere in the project.

## Enabling Pass Debug Information

The Pass debug information can be enabled by passing in the flag `--debug` to the `DEBUG_FLAG`
variable within the Makefile. Simply remove the flag if no debug information is needed.
