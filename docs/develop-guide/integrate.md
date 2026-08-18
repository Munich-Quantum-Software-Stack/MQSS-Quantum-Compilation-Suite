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
  GIT_TAG        <commit-hash or Release-tag>.  # e.g. v2.0.0
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
with a preset optimization level and a known output format, `mqss::MQSSCompiler` wraps all of this
behind a single `compile()` call. See [Using MQSSCI as a Library](../user_guide/library.md) for that
path; come back here if you need something it doesn't cover.

### Including the Headers

Four headers are relevant to consumers, split across two namespaces — `mqss::opt` for
dialect-agnostic optimization and `mqss::codegen` for lowering to exchange formats:

- `Passes/Transforms/Dialects.h` (`mqss::opt`) — registers the MLIR dialects (Quake, Catalyst's
  quantum dialect, and the standard MLIR dialects the passes lower into) that the passes need to
  understand your program. Include this before parsing or building any MLIR module.
- `Passes/Transforms/Transforms.h` (`mqss::opt`) — declares every individual transformation pass
  factory function, for building a custom pass pipeline pass-by-pass.
- `Passes/Transforms/Pipelines.h` (`mqss::opt`) — declares the three preset optimization levels
  `O1`, `O2` and `O3`, for appending a whole preset stage to an `mlir::OpPassManager` in one call.
- `Passes/CodeGen/CodeGenPasses.h` (`mqss::codegen`) — declares every individual code-generation
  pass factory function (lowering to exchange formats such as `QIR` or `OpenQASM`), for building a
  custom pass pipeline pass-by-pass.

## Declaring and Using a Pass Pipeline

A pipeline is simply a sequence of passes appended to an `mlir::OpPassManager`. Which pass factory
function you call depends on whether the pass takes options.

### Using a Preset Pipeline Directly in Your Project

Call one of the preset optimization pipelines — `O1`, `O2`, or `O3` — directly:

```cpp
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"
#include "Passes/CodeGen/CodeGenPasses.h"

// 1. Get an MLIRContext with every dialect MQSSCI's passes need already loaded.
std::unique_ptr<mlir::MLIRContext> context = mqss::opt::createMQSSContext();

//  2. Parse the source dialect and create MLIR module
auto module = mlir::parseSourceFile<mlir::ModuleOp>(src_path, context.get());
if (!module) {
  llvm::errs() << "failed to parse MLIR file\n";
}

// 3. Declare the MLIR Pass Manager
mlir::PassManager pm(context.get());

// 4. Add passes from the O1 pass pipeline
mqss::opt::O1(pm);

// Run the passes
if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Pipeline failed\n";
}

// 5. Add a CodeGen Pass and RUN the pass
pm.addPass(mqss::codegen::QuakeToQASM2Pass());
if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Conversion of Quake to QASM2 failed\n";
}
```

`mqss::opt::createMQSSContext()` registers everything MQSSCI's passes need to understand your
program (Quake, Catalyst's quantum dialect, and the standard MLIR dialects the passes lower into)
and returns a ready-to-use `MLIRContext` in one call — equivalent to building a `DialectRegistry`
with `registerMQSSDialects`, constructing an `MLIRContext` from it, and calling
`loadAllAvailableDialects()` yourself. Keep the returned context alive for as long as you're parsing
MLIR or running passes.

Please refer to `Passes/Transforms/Pipelines.h` for details on which specific passes the pipelines
include.

## Declaring a custom Pass Pipeline

If the presets don't fit your use case, assemble your own pipeline by adding individual passes to an
`mlir::OpPassManager` one at a time, in whatever order you need. Every pass factory lives in
`Passes/Transforms/Transforms.h` and `Passes/CodeGen/CodeGenPasses.h`, and falls into one of two
shapes depending on whether the pass takes options.

### Passes Without Options

Passes with no configurable options are constructed by calling their factory function directly:

```cpp
#include "Passes/Transforms/Transforms.h"

mlir::OpPassManager pm;
pm.addPass(mqss::opt::CommonCNOTReversePass());
pm.addPass(mqss::opt::CommonNormalizeArgAnglePass());
```

Check `Transforms.h` for other option-free passes.

### Passes With Options

Passes that expose options have a companion `<PassName>Options` struct. Populate it and pass it to
the corresponding `create<PassName>` factory. For example:

```cpp
#include "Passes/Transforms/Transforms.h"

mlir::OpPassManager pm;

CommonGateCancellationPassOptions cancelOpts;
cancelOpts.mode = "CancelGate";
pm.addPass(mqss::opt::createCommonGateCancellationPass(cancelOpts));

CommonCommutePassOptions commuteOpts;
commuteOpts.mode = "CX-RX";
pm.addPass(mqss::opt::createCommonCommutePass(commuteOpts));
```

Other option-bearing passes: `CommonSwitchPass`, `CommonReductionPass`, `CommonDecompositionPass`,
`CommonMappingPass`, `BasisConversionPass`. See [Passes](passes.md) for the full list of valid
option values for each.

### Example: The O2 Pipeline

The built-in `O2` optimization level combines both styles above into a single reusable pipeline
(`lib/Passes/Transforms/Pipeline.cpp`):

```cpp
void mqss::opt::O2(mlir::OpPassManager &pm) {
  CommonGateCancellationPassOptions CancelOpts;
  CommonCommutePassOptions CommuteOpts;
  CancelOpts.mode = "CancelGate";
  CommuteOpts.mode = "CX-RX";

  pm.addPass(createCommonGateCancellationPass(CancelOpts));
  pm.addPass(CommonCNOTReversePass());
  pm.addPass(createCommonCommutePass(CommuteOpts));

  // Standard MLIR passes
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
}
```
