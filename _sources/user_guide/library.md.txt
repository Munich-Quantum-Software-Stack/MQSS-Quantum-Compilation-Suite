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

# Using MQSSCI as a Library

The [Passes](passes.md) page shows how to run passes through the `mqss-opt`/`mqss-cc` command-line
tools. This page is for the case where you don't want a separate command-line step at all — you want
your own C++ program to load an MLIR module and run MQSSCI passes on it directly.

The two are not different features: `mqss-opt` itself is just a small C++ program that does exactly
what's shown below. Calling it from your own code gives you the same passes, without shelling out to
a separate binary.

## Before You Start

Your project needs to link against the MQSSCI library first. That's a one-time CMake setup step —
see
[Integrating the MQSS Quantum Compilation Suite within your project](../develop-guide/integrate.md)
for how to fetch MQSSCI with `FetchContent` and link your target against `mqss-ci::mqss-ci`. The
rest of this page assumes that step is done and focuses purely on the C++ side.

## A Minimal Example

Here's the smallest useful program: load an MLIR file, run the `O1` preset pipeline on it, then
lower the result to OpenQASM 2.

```cpp
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"

// 1. Tell MLIR about the dialects MQSSCI's passes need (Quake, Catalyst-quantum, etc.)
mlir::DialectRegistry registry;
mqss::opt::registerMQSSDialects(registry);
mlir::MLIRContext context(registry);
context.loadAllAvailableDialects();

// 2. Parse your MLIR file into a module
auto module = mlir::parseSourceFile<mlir::ModuleOp>(src_path, &context);
if (!module) {
  llvm::errs() << "failed to parse MLIR file\n";
}

// 3. Build a pass manager and add a preset optimization pipeline
mlir::PassManager pm(&context);
mqss::opt::O1(pm);

if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Pipeline failed\n";
}

// 4. Lower the optimized module to OpenQASM 2
pm.addPass(mqss::opt::QuakeToQASM2Pass());
if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Conversion of Quake to QASM2 failed\n";
}
```

Walking through it:

1. `registerMQSSDialects` fills in a `DialectRegistry` with every MLIR dialect the passes operate on
   or lower into. Do this before parsing anything — the parser needs to recognize the dialect
   operations in your input.
2. `mlir::parseSourceFile` reads your MLIR/Quake/Catalyst-quantum source file into an in-memory
   module.
3. `mqss::opt::O1(pm)` appends the whole `O1` preset pipeline to the pass manager in one call. `O2`
   and `O3` work the same way. See [Pass Pipelines](passes.md#pass-pipelines) for what each preset
   includes.
4. `mqss::opt::QuakeToQASM2Pass()` is a code-generation pass — it's added and run separately from
   the optimization pipeline, same as chaining `--O1 --quake-to-qasm2` on the `mqss-opt` command
   line.

If you'd rather capture the OpenQASM output in a string instead of printing it, `QuakeToQASM2Pass`
also accepts an `llvm::raw_ostream&`:

```cpp
std::string qasm;
llvm::raw_string_ostream os(qasm);
pm.addPass(mqss::opt::QuakeToQASM2Pass(os));
```

## Invoking Passes: Same Rules as the Developer Guide

Everything beyond this minimal example — running individual passes instead of a preset, passing
options to a pass (e.g. `CommonGateCancellationPassOptions`), or assembling a fully custom pipeline
— works exactly as described in
[Integrating the MQSS Quantum Compilation Suite within your project](../develop-guide/integrate.md#declaring-a-custom-pass-pipeline).
There's no separate API for library users: whether you're writing a pass yourself or just consuming
the library, you build the same `mlir::OpPassManager` and call the same factory functions from
`Passes/Transforms/Transforms.h`.

For the full list of available passes, their options, and valid option values, see
[Passes](passes.md).
