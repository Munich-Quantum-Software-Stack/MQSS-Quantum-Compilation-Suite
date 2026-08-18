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
your own C++ program to compile a circuit directly.

## Before You Start

Your project needs to link against the MQSSCI library first. That's a one-time CMake setup step —
see
[Integrating the MQSS Quantum Compilation Suite within your project](../develop-guide/integrate.md)
for how to fetch MQSSCI with `FetchContent` and link your target against `mqss-ci::mqss-ci`. The
rest of this page assumes that step is done and focuses purely on the C++ side.

## A Minimal Example

`mqss::MQSSCompiler` is the simplest way to compile a circuit: one header, one class, one call. It
hides dialect registration, MLIR context setup, and pipeline assembly behind a single `compile`
call.

```cpp
#include "MQSSCIInterfaces/MQSSCompiler.h"

mqss::MQSSCompiler compiler;

mqss::CompilerOptions opts;
opts.optimization_level = 1;     // 1, 2, or 3 — selects the O1/O2/O3 preset pipeline
opts.result_type = "OpenQASM2";  // or a QIR profile, e.g. "qir-base:2.0"

std::optional<std::string> qasm = compiler.compile("circuit.qke", "planqc", opts);
if (!qasm) {
  // Compilation failed. MQSSCompiler has already reported a diagnostic through
  // MLIR's diagnostic engine — it never throws or aborts the process.
  return 1;
}

llvm::outs() << *qasm;
```

Walking through it:

1. `#include "MQSSCIInterfaces/MQSSCompiler.h"` is the only header you need for this path.
2. `mqss::CompilerOptions` configures the run: `optimization_level` selects the `O1`/`O2`/`O3`
   preset pipeline (see [Pass Pipelines](passes.md#pass-pipelines) for what each includes), and
   `result_type` selects the output format — `"OpenQASM2"` or a QIR profile string of the form
   `profile:version`, where `profile` is one of `qir`, `qir-base`, `qir-adaptive`, `qir-full` and
   `version` is `2.0` or `2.1` (e.g. `"qir-base:2.0"`).
3. The second argument to `compile` (`"planqc"` above) is a known backend name that selects a
   built-in native-gate set for decomposition. See [Choosing a Backend](#choosing-a-backend) below
   for the alternatives.
4. `compile` returns `std::optional<std::string>`, not the compiled circuit directly. On success, it
   holds the compiled output; on failure it's `std::nullopt`, and the reason has already been
   reported via `mlir::emitError` — MQSSCI is built without C++ exceptions, so a failed compile
   never throws or crashes your program, it just returns an empty optional.

## Choosing a Backend

`compile` has overloads for three ways to select the native-gate set used for decomposition: by a
known backend name, by an explicit gate list, or not at all:

```cpp
// By backend name: iqm, planqc, and wmi are recognized out of the box.
compiler.compile("circuit.qke", "iqm", opts);

// By an explicit native-gate set, when your target isn't one of the built-in backends.
compiler.compile("circuit.qke", {"rz", "rx", "cz"}, opts);

// Neither: skip native-gate decomposition entirely.
compiler.compile("circuit.qke", opts);
```

| `backend_name` | Native-gate set      |
| -------------- | -------------------- |
| `"iqm"`        | `phased_rx`, `cz`    |
| `"planqc"`     | `rx`, `cz`, `rz`     |
| `"wmi"`        | `cz`, `x`, `y`, `rz` |

The full signature (used in the examples above via its three shorthand overloads) also accepts a
`qubit_connectivity` map alongside the native-gate set, for targets with restricted qubit
connectivity. See `MQSSCIInterfaces/MQSSCompiler.h` for all four `compile` overloads.

## Advanced: Building a Custom Pipeline Yourself

`MQSSCompiler` covers the common case: a preset optimization level, a fixed native-gate set, and one
of two output formats. If you need to run individual passes, pass pass-specific options (e.g.
`CommonGateCancellationPassOptions`), or assemble a pipeline `MQSSCompiler` doesn't support, drop
down to the same `mlir::OpPassManager`-based API `MQSSCompiler` itself is built on:

```cpp
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"
#include "Passes/CodeGen/CodeGenPasses.h"

// 1. Get an MLIRContext with every dialect MQSSCI's passes need already loaded.
std::unique_ptr<mlir::MLIRContext> context = mqss::opt::createMQSSContext();

// 2. Parse your MLIR file into a module
auto module = mlir::parseSourceFile<mlir::ModuleOp>(src_path, context.get());
if (!module) {
  llvm::errs() << "failed to parse MLIR file\n";
}

// 3. Build a pass manager and add a preset optimization pipeline
mlir::PassManager pm(context.get());
mqss::opt::O1(pm);

if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Pipeline failed\n";
}

// 4. Lower the optimized module to OpenQASM 2
pm.addPass(mqss::codegen::QuakeToQASM2Pass());
if (mlir::failed(pm.run(*module))) {
  llvm::errs() << "Compiler: Conversion of Quake to QASM2 failed\n";
}
```

`mqss::opt::createMQSSContext()` is a convenience that registers every MQSSCI dialect and returns a
ready-to-use `MLIRContext` in one call — equivalent to building a `DialectRegistry` with
`registerMQSSDialects`, constructing an `MLIRContext` from it, and calling
`loadAllAvailableDialects()` yourself.

If you'd rather capture the OpenQASM output in a string instead of printing it, `QuakeToQASM2Pass`
also accepts an `llvm::raw_ostream&`:

```cpp
std::string qasm;
llvm::raw_string_ostream os(qasm);
pm.addPass(mqss::codegen::QuakeToQASM2Pass(os));
```

Everything beyond this — passing options to a pass, or assembling a fully custom pipeline — works
exactly as described in
[Integrating the MQSS Quantum Compilation Suite within your project](../develop-guide/integrate.md#declaring-a-custom-pass-pipeline).
There's no separate API for library users: whether you're writing a pass yourself or just consuming
the library, you build the same `mlir::OpPassManager` and call the same factory functions from
`Passes/Transforms/Transforms.h` and `Passes/CodeGen/CodeGenPasses.h`.

For the full list of available passes, their options, and valid option values, see
[Passes](passes.md).
