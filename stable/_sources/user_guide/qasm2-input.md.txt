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

# OpenQASM 2 as Input

MQSSCI's passes operate on MLIR, but circuits are often written as OpenQASM 2 text. The QASM2
frontend translates an OpenQASM 2 program into the CUDA-Q Quake dialect, so it can then go through
the same passes and pipelines as any other Quake circuit. It is available in three forms:

- the standalone **`mqss-translate`** tool, built next to `mqss-opt`;
- the **`MQSSCompiler`** C++ interface, whose `compile`/`compileSource` accept OpenQASM 2 directly;
- the **`translateQASM2ToQuake`** C++ function, for callers who only want the Quake module.

The translation is a _translation_, not a pass: a QASM2 string is not an MLIR module, so there is
nothing for a pass to run on. It is the inverse direction of the `quake-to-qasm2` pass.

## Supported OpenQASM 2 Subset

The translator is deliberately small. It currently handles circuits such as Bell and GHZ states:

| Construct               | Support                                                       |
| ----------------------- | ------------------------------------------------------------- |
| `OPENQASM 2.x;`         | Accepted; other versions are rejected. See the caveats below. |
| `include "qelib1.inc";` | Accepted and ignored; the supported gates are built in.       |
| `qreg name[n];`         | Supported. All `qreg`s share one `quake.alloca`.              |
| `creg name[n];`         | Supported, but only checked as the target of a `measure`.     |
| `h x y z s sdg t tdg`   | Supported, applied to an indexed qubit, e.g. `h q[0];`.       |
| `cx`, `cz`              | Supported, as a controlled `quake.x` / `quake.z`.             |
| `measure q[i] -> c[j];` | Supported. Emits one `quake.mz` on that qubit.                |
| `measure q -> c;`       | Supported for equal-size registers; see the caveats below.    |
| `// comments`           | Supported.                                                    |

Everything else is rejected with an error (see [Caveats and Limitations](#caveats-and-limitations)).

## Using mqss-translate

`mqss-translate` is built together with `mqss-opt` (see [build](build.md)) and installed into the
same `INSTALL_DIR`, so it is on your `$PATH` after `eval "$(make set-target-paths)"`. It is built on
MLIR's `mlir-translate` driver, so it accepts the usual `-o`, `--help` and `-` (stdin) arguments.

Take the Bell state `bell.qasm`:

```text
OPENQASM 2.0;
include "qelib1.inc";
qreg q[2];
creg c[2];
h q[0];
cx q[0],q[1];
measure q[0] -> c[0];
measure q[1] -> c[1];
```

Translate it with:

```sh
mqss-translate --import-qasm2 bell.qasm
```

The result is a Quake module along these lines (SSA value names may differ):

```llvm
module {
  func.func @__nvqpp__mlirgen__qasm2_kernel() attributes {"cudaq-entrypoint", "cudaq-kernel"} {
    %0 = quake.alloca !quake.veq<2>
    %1 = quake.extract_ref %0[0] : (!quake.veq<2>) -> !quake.ref
    %2 = quake.extract_ref %0[1] : (!quake.veq<2>) -> !quake.ref
    quake.h %1 : (!quake.ref) -> ()
    quake.x [%1] %2 : (!quake.ref, !quake.ref) -> ()
    %measOut = quake.mz %1 : (!quake.ref) -> !quake.measure
    %measOut_0 = quake.mz %2 : (!quake.ref) -> !quake.measure
    return
  }
}
```

Each `measure q[i] -> c[j];` statement becomes one `quake.mz` on that qubit (`%1` is `q[0]`, `%2` is
`q[1]`). A register-wide `measure q -> c;` is instead a single measurement of the whole register, as
in CUDA-Q's own output:

```llvm
%measOut = quake.mz %0 : (!quake.veq<3>) -> !cc.stdvec<!quake.measure>
```

This applies when `q` is the whole allocation, i.e. the program's only `qreg`. If the program has
several `qreg`s, `measure q -> c;` on one of them is emitted as one `quake.mz` per qubit instead.
The classical register itself is not represented in the Quake output.

The program becomes a single entry-point kernel named `__nvqpp__mlirgen__qasm2_kernel`, with the
`cudaq-entrypoint` and `cudaq-kernel` attributes the rest of MQSSCI expects.

Because the output is ordinary Quake text, it can be piped straight into `mqss-opt`:

```sh
mqss-translate --import-qasm2 bell.qasm | mqss-opt --BasisConversionPass=gates=prx,cz
```

See [Running test circuits](running.md) for what `mqss-opt` can do with it.

## Using the MQSSCompiler Interface

`MQSSCompiler::compile` and `MQSSCompiler::compileSource` (and all their overloads) accept OpenQASM
2 in place of a Quake file or string. No extra option is needed:

```cpp
#include "MQSSCIInterfaces/MQSSCompiler.h"

mqss::mqssci::MQSSCompiler compiler;

mqss::mqssci::CompilerOptions opts;
opts.optimization_level = mqss::mqssci::OptLevel::O1;
opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

const std::string qasm2 = "OPENQASM 2.0;\n"
                          "include \"qelib1.inc\";\n"
                          "qreg q[2];\n"
                          "creg c[2];\n"
                          "h q[0];\n"
                          "cx q[0],q[1];\n"
                          "measure q -> c;\n";

std::optional<std::string> compiled =
    compiler.compileSource(qasm2, {"rx", "cz", "rz"}, opts);
// Or, from a file: compiler.compile("path/to/bell.qasm", {"rx", "cz", "rz"}, opts);
```

The input format is detected from the content. If the first token of the input, after whitespace and
`//` comments, is `OPENQASM`, the text is translated to Quake first. Anything else is parsed as
MLIR, exactly as before. The file extension is not consulted, so `compile` and `compileSource`
behave the same way. From there the circuit goes through the normal pipeline: optimization, qubit
mapping, native-gate decomposition and lowering to `result_format`. The `backend_name`, native-gate
set and `qubit_connectivity` arguments work as described in [Using MQSSCI as a Library](library.md).

An input that starts with `OPENQASM` but uses a version other than 2.x, or that falls outside the
supported subset above, makes `compile`/`compileSource` return `std::nullopt` after reporting a
diagnostic, like any other failure.

### Getting the Quake module only

To get the translated Quake without compiling it, call `translateQASM2ToQuake`. It returns the
module's textual MLIR form, or `std::nullopt` if the translation failed:

```cpp
std::optional<std::string> quake = compiler.translateQASM2ToQuake(qasm2);
```

`mqss::mqssci::translation::translateQASM2ToQuake(llvm::StringRef, mlir::MLIRContext &)`, declared
in `Translation/QASM2ToQuake.h`, is the lower-level entry point. It returns an `mlir::ModuleOp` in a
context you provide, and is what `mqss-translate` and `MQSSCompiler` are both built on.

`MQSSCompiler::getSupportedInputFormats()` lists OpenQASM 2 alongside the Quake and Catalyst
formats; see [Discovering Supported Input Formats](library.md#discovering-supported-input-formats).

## Caveats and Limitations

The translator covers a small, fixed subset of OpenQASM 2. Keep the following in mind.

### Language coverage

- **Only a fixed gate set.** The accepted gates are exactly `h x y z s sdg t tdg cx cz`. Any other
  gate — including ones `qelib1.inc` defines, such as `rx`, `u1`, `swap` or `ccx` — is rejected as
  unsupported.
- **No parameterized gates or expressions.** Gates with parameters, `U`/`u1`/`u2`/`u3`, and
  arithmetic expressions such as `pi/2` are not handled.
- **No user-defined or opaque gates.** `gate` and `opaque` declarations are rejected.
- **No `barrier`, `reset` or `if`.** These are errors, not silently dropped, so no classical control
  flow is available.
- **Gates act on indexed qubits only.** `h q;` (broadcast over a register) is rejected; write
  `h q[0];`. `measure` is the one statement that accepts a whole register.
- **`include` is not read.** Any `include` statement is accepted and ignored. The file is never
  opened, so the gates listed above are the only ones available, whatever it names.
- **Only `//` comments.** Block comments (`/* ... */`) are not recognized and produce an error.
- **Keywords are case-sensitive**, and the version header must be spelled `OPENQASM`.

### Registers and measurements

- **`qreg` order.** Every `qreg` must be declared before the first gate or measurement. A register
  name, quantum or classical, can only be declared once.
- **Registers are flattened.** All `qreg`s share a single `quake.alloca`, laid out in declaration
  order. Register names and boundaries are not kept in the Quake output.
- **Classical registers are not represented.** A `creg` is only checked to exist and to be large
  enough. The Quake output carries no classical-register name and no classical bit index, so
  `measure q[0] -> c[1];` and `measure q[0] -> c[0];` produce identical Quake. Measurements appear
  in statement order, and their results are not consumed by any other operation.
- **Register-wide `measure` depends on the program.** `measure q -> c;` becomes a single `veq`
  measurement only when `q` is the program's entire allocation, i.e. its only `qreg`. With several
  `qreg`s it is emitted as one `quake.mz` per qubit.
- **Mismatched registers are errors.** `measure` needs both operands indexed or both whole, and
  whole registers must be the same size.

### Output shape

- **One kernel.** The program becomes a single function, `__nvqpp__mlirgen__qasm2_kernel`, marked
  `cudaq-entrypoint` and `cudaq-kernel`. Module-level attributes that CUDA-Q's own frontend adds
  (data layout, target triple, `quake.mangled_name_map`) are not emitted.
- **Round trips do not reproduce the input text.** Translating to Quake and lowering back with
  `quake-to-qasm2` yields a normalized program: registers are renamed, e.g. `var0`, and gates may be
  rewritten by the pipeline. Compare circuits by behavior, not by text.

### Detection and diagnostics

- **`MQSSCompiler` recognizes QASM2 by its header.** Input is treated as OpenQASM only if its first
  token, after whitespace and `//` comments, is `OPENQASM`. A QASM file without the header is parsed
  as MLIR and fails with an MLIR parse error. `mqss-translate --import-qasm2` does not require the
  header, since you have already said the input is QASM.
- **Only the version prefix is checked.** `OPENQASM 2.0;` and `2.x` headers pass; anything else,
  such as `OPENQASM 3.0;`, is rejected. OpenQASM 3 syntax is not supported.
- **The first error stops translation.** The translator reports one diagnostic and gives up, so
  fixing it may reveal the next one. Through `MQSSCompiler` it is followed by a generic "failed to
  parse input circuit" message.
- **Locations are line and column within the text.** Diagnostics point to `<qasm2>:line:col`, not to
  the file path, even when the text came from a file passed to `compile`.
- **`InputFormat::QASM2` is for reporting.** It makes `getSupportedInputFormats()` list OpenQASM 2,
  but it is not an option you set: the format is always detected from the content.

## Tests

The OpenQASM 2 path is covered by:

- unit tests in `tests/unittests/MQSSCIInterfaces/test_Interface.cpp`, which compile the Bell and
  GHZ fixtures `tests/unittests/input/two_qubit_bell.qasm` and `ghz.qasm` through `compileSource`
  and check the rejected inputs. Run them with `make test-interfaces`;
- a lit test for the tool, `tests/dialects/quake/QASM2ToQuakeBell.test`, run as part of
  `check-mqss`.
