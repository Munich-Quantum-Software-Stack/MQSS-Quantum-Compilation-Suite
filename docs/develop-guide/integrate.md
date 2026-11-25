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

# Integrating the MQSS Quantum Compilation Suite within your project

MQSSCI (MQSS-Quantum-Compilation-Suite) is a library of [MLIR](https://mlir.llvm.org/) compiler
passes for transforming and optimizing quantum programs. This guide walks through adding MQSSCI to
your own CMake project, linking against it, and building a pass pipeline — no prior compiler
background required.

## Using MQSSCI as a CMake Dependency

MQSSCI (MQSS-Quantum-Compilation-Suite) can be integrated into any C++ project as an external CMake
module using `FetchContent`. E.g. within `FindMQSSCI.cmake`:

```cmake
set(CUDAQ_AUTO_FETCH ON CACHE BOOL "" FORCE)
set(CATALYST_AUTO_FETCH ON CACHE BOOL "" FORCE)

FetchContent_Declare(mqssci
  GIT_REPOSITORY https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite.git
  GIT_TAG        <commit-hash or Release-tag>.  # e.g. v2.1.0
)
FetchContent_MakeAvailable(mqssci)
```

Then, within the appropriate `CMakeLists.txt`:

```cmake
find_package(MQSSCI REQUIRED)   # if the file above is named FindMQSSCI.cmake
```

Note: `find_package(MQSSCI REQUIRED)` only resolves if `CMAKE_MODULE_PATH` includes the directory
containing `FindMQSSCI.cmake`, e.g.:

```cmake
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
```

### Linking Against the Pass Library

Link your target against the `mqss-ci::mqss-ci` alias target:

```cmake
target_link_libraries(<your-target>
  PRIVATE
    mqss-ci::mqss-ci
)
```

This single target is sufficient — `mqss-ci::mqss-ci` publicly exports every include directory it
needs (its own `include/`, generated TableGen headers, QDMI, MQT-QMAP, and the MLIR/LLVM headers),
so no manual include-path bookkeeping is required in the consuming project. It also gives you
`MQSSCIInterfaces/MQSSCompiler.h` (see below) for free — no separate target to link.

### Prefer a Simpler API?

Everything below this point is the lower-level, `mlir::OpPassManager`-based API — full control over
dialect registration, individual passes, and pass options. If you just want to compile a circuit
with a preset optimization level and a known output format, `mqss::mqssci::MQSSCompiler` wraps all
of this behind a single `compile()` call. See [Using MQSSCI as a Library](../user_guide/library.md)
for that path; come back here if you need something it doesn't cover.

### Including the Headers

Five headers are relevant to consumers, split across namespaces — `mqss::mqssci::opt` for
dialect-agnostic optimization and pipelines, `mqss::mqssci::codegen` for lowering to exchange
formats and `mqss::mqssci::translation` for the text-format translations:

- `Passes/Transforms/Dialects.h` (`mqss::mqssci::opt`) — registers the MLIR dialects (Quake,
  Catalyst's quantum dialect, and the standard MLIR dialects the passes lower into) that the passes
  need to understand your program. Include this before parsing or building any MLIR module.
- `Passes/Transforms/TransformPasses.h` (`mqss::mqssci::opt`) — declares every individual
  transformation pass factory function, for building a custom pass pipeline pass-by-pass.
- `Passes/Pipelines/Pipelines.h` (`mqss::mqssci::pipeline`) — declares the three preset optimization
  levels `O1`, `O2` and `O3`, and the pipelines that build the output stage, `QASMLoweringPipeline`
  and `QIRConversionPipeline`, for appending a whole stage to an `mlir::OpPassManager` in one call.
- `Passes/CodeGen/CodeGenPasses.h` (`mqss::mqssci::codegen`) — declares every individual
  code-generation pass factory function (lowering to exchange formats such as `QIR` or `OpenQASM`),
  for building a custom pass pipeline pass-by-pass.
- `Translation/MLIRToTarget.h` and `Translation/QASM2ToQuake.h` (`mqss::mqssci::translation`) —
  register the `mlir-translate` translations (`registerMLIRToTargetTranslations()`,
  `registerQASM2ToQuakeTranslation()`), for building your own translation tool the way
  `mqss-translate` is built, and declare `translateQASM2ToQuake`. `makeQIRTranslation(profile)` is
  also declared there; `MQSSCompiler` uses it for its QIR output formats, so the two always produce
  the same QIR.

## Declaring and Using a Pass Pipeline

A pipeline is simply a sequence of passes appended to an `mlir::OpPassManager`. Which pass factory
function you call depends on whether the pass takes options.

### Using a Preset Pipeline Directly in Your Project

Call one of the preset optimization pipelines — `O1`, `O2`, or `O3` — directly:

```cpp
#include "Passes/Transforms/Dialects.h"
#include "Passes/Pipelines/Pipelines.h"
#include "Passes/CodeGen/CodeGenPasses.h"

// 1. Get an MLIRContext with every dialect MQSSCI's passes need already loaded.
std::unique_ptr<mlir::MLIRContext> context = mqss::mqssci::opt::createMQSSContext();

//  2. Parse the source dialect and create MLIR module
auto module = mlir::parseSourceFile<mlir::ModuleOp>(src_path, context.get());
if (!module) {
  llvm::errs() << "failed to parse MLIR file\n";
}

// 3. Declare the MLIR Pass Manager
mlir::PassManager pm(context.get());

// 4. Add passes from the O1 pass pipeline
mqss::mqssci::pipeline::O1(pm);

// Run the passes
if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Pipeline failed\n";
}

// 5. Append the OpenQASM output pipeline (cse, canonicalize, MLIRToQASMPass), RUN it and keep the text
std::string qasm;
llvm::raw_string_ostream os(qasm);
mlir::PassManager lowering(context.get());
mqss::mqssci::pipeline::QASMLoweringPipeline(lowering, /*qasm3=*/false, os);
if (mlir::failed(lowering.run(*module))) {
  llvm::errs() << "Compiler: Conversion of the module to OpenQASM failed\n";
}
```

The module must already be in the target's gate set; native-gate decomposition is the
`BasisConversionPass` (`mqss::mqssci::codegen::createBasisConversionPass`), which is not part of
`O1`/`O2`/`O3`.

`mqss::mqssci::opt::createMQSSContext()` registers everything MQSSCI's passes need to understand
your program (Quake, Catalyst's quantum dialect, and the standard MLIR dialects the passes lower
into) and returns a ready-to-use `MLIRContext` in one call — equivalent to building a
`DialectRegistry` with `registerMQSSDialects`, constructing an `MLIRContext` from it, and calling
`loadAllAvailableDialects()` yourself. Keep the returned context alive for as long as you're parsing
MLIR or running passes.

Please refer to `Passes/Pipelines/Pipelines.h` for details on which specific passes the pipelines
include.

## Declaring a custom Pass Pipeline

If the presets don't fit your use case, assemble your own pipeline by adding individual passes to an
`mlir::OpPassManager` one at a time, in whatever order you need. Every pass factory lives in
`Passes/Transforms/TransformPasses.h` and `Passes/CodeGen/CodeGenPasses.h`, and falls into one of
two shapes depending on whether the pass takes options.

### Passes Without Options

Passes with no configurable options are constructed by calling their factory function directly:

```cpp
#include "Passes/Transforms/TransformPasses.h"

mlir::OpPassManager pm;
pm.addPass(mqss::mqssci::opt::CommonCNOTReversePass());
pm.addPass(mqss::mqssci::opt::CommonNormalizeArgAnglePass());
```

Check `TransformPasses.h` for other option-free passes.

### Passes With Options

Passes that expose options have a companion `<PassName>Options` struct. Populate it and pass it to
the same factory name. For example:

```cpp
#include "Passes/Transforms/TransformPasses.h"

mlir::OpPassManager pm;

CommonGateCancellationPassOptions cancelOpts;
cancelOpts.mode = "CancelGate";
pm.addPass(mqss::mqssci::opt::CommonGateCancellationPass(cancelOpts));

CommonCommutePassOptions commuteOpts;
commuteOpts.mode = "CX-RX";
pm.addPass(mqss::mqssci::opt::CommonCommutePass(commuteOpts));
```

Other option-bearing passes: `CommonSwitchPass`, `CommonReductionPass` and `CommonMappingPass` (in
`mqss::mqssci::opt`), and `BasisConversionPass`, whose factory is
`mqss::mqssci::codegen::createBasisConversionPass(options)`. See [Passes](../user_guide/passes.md)
for the full list of valid option values for each.

### Example: The O2 Pipeline

The built-in `O2` optimization level combines both styles above into a single reusable pipeline
(`lib/Passes/Pipelines/Pipeline.cpp`):

```cpp
void mqss::mqssci::pipeline::O2(mlir::OpPassManager &pm) {
  CommonGateCancellationPassOptions CancelOpts;
  CommonCommutePassOptions CommuteOpts;
  CancelOpts.mode = "CancelGate";
  CommuteOpts.mode = "CX-RX";

  pm.addPass(CommonGateCancellationPass(CancelOpts));
  pm.addPass(CommonCNOTReversePass());
  pm.addPass(CommonCommutePass(CommuteOpts));

  // Standard MLIR passes
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}
```

## Testing the Interface

`mqss::mqssci::MQSSCompiler` has its own GoogleTest suite, separate from the lit/FileCheck tests
that cover the passes themselves (see [Testing](develop-guide.md#testing)). It lives in
`tests/unittests/MQSSCIInterfaces/test_Interface.cpp` and exercises the library end to end: it
compiles real fixture circuits from `tests/unittests/input` through both `compile` and
`compileSource` APIs, and checks properties of the emitted output. The fixtures cover Quake
(`two_qubit_bell.qke`, `ghz.qke`), Catalyst-quantum (`ghz_catalyst.mlir`) and OpenQASM 2
(`two_qubit_bell.qasm`, `ghz.qasm`) inputs, compiled to OpenQASM 2 and 3, QIR and IQM JSON, and the
inputs that must be rejected.

`BUILD_INTERFACES_TESTS` is `ON` by default in `scripts/build.sh`, so `make build && make target`
(or an equivalent manual `cmake`/`ninja` build) builds `MQSSCIInterface_test` automatically — no
extra flag needed. Test the suite with:

```sh
make test-interfaces      # wraps: ctest --test-dir build --output-on-failure
```

or scope it to just this suite with `ctest --test-dir build -R MQSSCIInterface --output-on-failure`.

If you add a new `compile`/`compileSource` overload or change how `MQSSCompiler` handles a
particular option, extend this suite rather than relying solely on the pass-level lit tests — lit
tests catch regressions in individual passes, but only this suite exercises the library's public C++
API as a consumer would actually call it.
