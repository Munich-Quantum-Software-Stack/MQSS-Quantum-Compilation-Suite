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

# Translating Circuits with mqss-translate

MQSSCI's passes operate on MLIR, but circuits are exchanged as text: OpenQASM, IQM JSON, QIR. The
`mqss-translate` tool converts between the two worlds, in both directions:

- **Import**: OpenQASM 2 text becomes a CUDA-Q Quake module (`--import-qasm2`).
- **Export**: a Quake or Catalyst-quantum module becomes OpenQASM 2/3, IQM JSON, QIR or LLVM IR
  (`--mlir-to-*`).

The typical workflow is a pipe: `mqss-opt` transforms the circuit, `mqss-translate` writes it out in
the format your target consumes.

```sh
mqss-opt circuit.qke --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-qasm3
```

## Why translations, not passes

Everything `mqss-translate` does is a MLIR _translation_, not a pass. A text format such as OpenQASM
is not an MLIR module, so there is nothing for a pass to run on, and `mlir-translate` (the driver
`mqss-translate` is built on) only runs translations; it never consults the pass registry. Passes
that rewrite MLIR into MLIR stay in `mqss-opt` (see [Passes](passes.md)). Code that writes a text
format is exposed here. Internally each translation runs a pass or pipeline from the `CodeGen`
library and streams its output to the tool's output, so the logic is the same one the `MQSSCompiler`
C++ interface uses.

`mqss-translate` is built together with `mqss-opt` (see [build](build.md)) and installed into the
same `INSTALL_DIR`, so it is on your `$PATH` after `eval "$(make set-target-paths)"`. It accepts the
usual `mlir-translate` arguments: `-o`, `--help` and `-` (stdin).

## Available translations

| Flag                        | Input               | Output                                   |
| --------------------------- | ------------------- | ---------------------------------------- |
| `--import-qasm2`            | OpenQASM 2 text     | Quake module                             |
| `--mlir-to-qasm2`           | Quake or Catalyst   | OpenQASM 2                               |
| `--mlir-to-qasm3`           | Quake or Catalyst   | OpenQASM 3                               |
| `--mlir-to-iqm-json`        | Quake               | IQM JSON circuit                         |
| `--mlir-to-qir`             | Quake or Catalyst   | QIR, `qir` (full) profile, version 2.0   |
| `--mlir-to-qir-base`        | Quake or Catalyst   | QIR, `qir-base` profile, version 2.0     |
| `--mlir-to-qir-adaptive`    | Quake or Catalyst   | QIR, `qir-adaptive` profile, version 2.0 |
| `--mlir-to-qir-full`        | Quake or Catalyst   | QIR, `qir-full` profile, version 2.0     |
| `--llvm-dialect-to-llvm-ir` | LLVM-dialect module | LLVM IR                                  |

Run `mqss-translate --help` for the authoritative list. Two things to keep in mind:

- The exports expect the circuit to be **already lowered to the target's gate set**. They do not
  decompose gates. Run `mqss-opt --BasisConversionPass=gates=...` first.
- **Catalyst-quantum input can be translated to OpenQASM and QIR, but not to IQM JSON.** The QIR
  translations lower Catalyst gates to calls into the Catalyst runtime (`__catalyst__qis__*`). The
  IQM JSON translation is Quake-only and reports an error on Catalyst input.

## Exporting to OpenQASM 2 and 3

Take the Bell state in the Quake dialect, decomposed into IQM's native gates and written as OpenQASM
3:

```sh
mqss-opt bell.qke --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-qasm3
```

```text
OPENQASM 3.0;
include "stdgates.inc";
gate r(param0,param1) q0 { U(param0,-pi/2 + param1,pi/2 - param1) q0; }
qubit[2] q;
bit[2] c;
r(1.5707963267949,1.5707963267949) q[0];
r(3.14159265358979,0) q[0];
r(1.5707963267949,1.5707963267949) q[1];
r(3.14159265358979,0) q[1];
cz q[0], q[1];
r(1.5707963267949,1.5707963267949) q[1];
r(3.14159265358979,0) q[1];
c[0] = measure q[0];
c[1] = measure q[1];
```

`--mlir-to-qasm2` produces the same circuit with OpenQASM 2 syntax. Catalyst-quantum input works
exactly the same way; replace `bell.qke` with a `.mlir` file in the `quantum` dialect.

### What the output looks like

|                    | OpenQASM 2                   | OpenQASM 3                     |
| ------------------ | ---------------------------- | ------------------------------ |
| Header and include | `OPENQASM 2.0;` `qelib1.inc` | `OPENQASM 3.0;` `stdgates.inc` |
| Registers          | `qreg q[N];` `creg c[M];`    | `qubit[N] q;` `bit[M] c;`      |
| Measurement        | `measure q[i] -> c[j];`      | `c[j] = measure q[i];`         |
| Whole register     | `measure q -> c;`            | `c = measure q;`               |

- **Register names are fixed**: the quantum register is always `q` and the classical register `c`.
  Their sizes are the allocation of the (possibly mapped) circuit and the number of measured bits.
- **Gates** are printed with full-precision angles. The supported gates are `h x y z s sdg t tdg`,
  `rx ry rz`, `swap`, `sx`/`sxdg`, `p` (the `R1` phase rotation), `U` (for `U2`/`U3`), `r` (the
  `PhasedRx`/`prx` gate), and the single-control gates `cx cy cz ch crx cry crz`.
- **Gates the standard include does not define are declared** at the top: `r` is declared as
  `gate r(param0,param1) q0 { ... }` whenever it is used, and `sxdg` is declared in OpenQASM 3
  output (`stdgates.inc` has `sx` but not `sxdg`). Each declaration appears once, after the include
  and before its first use.
- **Adjoints are honored.** `S†` and `T†` become `sdg`/`tdg`, an adjoint rotation (`RX`, `RY`, `RZ`,
  `R1`, `CRX`, `CRY`, `CRZ`) gets a negated angle, and self-inverse gates are unchanged.
- **Angles may be constant expressions.** Expressions such as `-pi / 2` written with `arith.negf`,
  `arith.divf`, `addf`, `subf`, `mulf` or `extf` of constants are folded to a literal.
- **Several kernels**: if the module holds more than one quantum kernel, one OpenQASM program is
  printed per kernel, in order.

### What is rejected

The translation fails with a diagnostic, rather than printing a circuit that is silently wrong, when
the input has something OpenQASM cannot express:

- A rotation angle that is not a compile-time constant (for example a function argument) fails with
  `No constant angle found to create RX gate!`.
- A quantum operation with no equivalent, such as `quantum.gphase`, `quantum.multirz` or a gate with
  two or more controls, fails with `operation 'quantum.gphase' has no OpenQASM equivalent`.
- A gate type the printer cannot write fails with
  `gate '<name>' is not supported by the OpenQASM backend`.
- A loop (`scf.for`, `scf.while`, `cc.loop`) or a conditional (`scf.if`, `cc.if`, `cf` branches)
  around quantum operations fails with
  `'scf.for' around quantum operations was not unrolled/folded`.
- A call that was not inlined (`func.call`) fails with `call 'func.call' was not inlined`.
- Dynamic qubit allocation (Catalyst) fails with `Dynamic qubit allocation is not supported`.

The input must therefore be straight-line code. The OpenQASM pipeline runs `cse` and `canonicalize`
before printing, so a loop that executes once, or a branch on a constant, folds away and is
accepted; anything else must be unrolled or inlined first. Automatic loop unrolling is not part of
the pipeline yet.

Two Catalyst gate names are not recognized by the extractor yet: `SX` and `R1`. A `quantum.custom`
with one of those names is rejected at analysis time.

## Exporting to IQM JSON, QIR and LLVM IR

```sh
# IQM JSON (Quake only)
mqss-opt bell.qke --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-iqm-json

# QIR, base profile (Quake only)
mqss-opt bell.qke --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-qir-base
```

`--mlir-to-iqm-json` first expands whole-register measurements into one measurement per qubit,
because every `"measure"` instruction in IQM JSON names exactly one qubit. The IQM emitter expects
IQM's native gate set (`prx`, `cz`). The QIR translations lower with the CUDA-Q pipeline and write
LLVM IR text; only version 2.0 of each profile is available from the command line (the C++
`QIRConversionPipeline` accepts `2.1` as well).

Catalyst-quantum circuits go to QIR with the same flags, for example:

```sh
mqss-opt circuit.mlir --BasisConversionPass=gates=rx,cz,rz --cse --canonicalize \
  | mqss-translate --mlir-to-qir-base
```

`--llvm-dialect-to-llvm-ir` on its own turns any LLVM-dialect MLIR module into LLVM IR text.

## Importing OpenQASM 2

The importer translates an OpenQASM 2 program into the CUDA-Q Quake dialect, so it can go through
the same passes and pipelines as any other Quake circuit. It is available in three forms:

- the **`mqss-translate --import-qasm2`** command;
- the **`MQSSCompiler`** C++ interface, whose `compile`/`compileSource` accept OpenQASM 2 directly;
- the **`translateQASM2ToQuake`** C++ function, for callers who only want the Quake module.

### Supported OpenQASM 2 subset

The importer is deliberately small. It currently handles circuits such as Bell and GHZ states:

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

### Using mqss-translate

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

Because the output is ordinary Quake text, it can be piped straight into `mqss-opt` and back out
through an export translation, which converts OpenQASM 2 into any of the formats above:

```sh
mqss-translate --import-qasm2 bell.qasm \
  | mqss-opt --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-qasm3
```

See [Running test circuits](running.md) for what `mqss-opt` can do with it.

## Using the MQSSCompiler Interface

`MQSSCompiler::compile` and `MQSSCompiler::compileSource` (and all their overloads) take a circuit
as a Quake or Catalyst-quantum MLIR file/string, or as OpenQASM 2. No extra option selects the input
format: it is detected from the content. The output format is `CompilerOptions::result_format`.

```cpp
#include "MQSSCIInterfaces/MQSSCompiler.h"

mqss::mqssci::MQSSCompiler compiler;

mqss::mqssci::CompilerOptions opts;
opts.optimization_level = mqss::mqssci::OptLevel::O1;
opts.result_format = mqss::mqssci::ResultFormat::OPENQASM3;

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

If the first token of the input, after whitespace and `//` comments, is `OPENQASM`, the text is
translated to Quake first. Anything else is parsed as MLIR. The file extension is not consulted, so
`compile` and `compileSource` behave the same way. From there the circuit goes through the normal
pipeline: optimization, qubit mapping, native-gate decomposition and lowering to `result_format`.
The `backend_name`, native-gate set and `qubit_connectivity` arguments work as described in
[Using MQSSCI as a Library](library.md).

Which input can go to which output:

| Input            | `OPENQASM2` / `OPENQASM3` | `IQMJSON` | `QIR*` |
| ---------------- | :-----------------------: | :-------: | :----: |
| Quake            |            yes            |    yes    |  yes   |
| OpenQASM 2       |      yes (via Quake)      |    yes    |  yes   |
| Catalyst-quantum |            yes            |    no     |  yes   |

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
context you provide, and is what `mqss-translate` and `MQSSCompiler` are both built on. The export
translations are registered by `mqss::mqssci::translation::registerMLIRToTargetTranslations()`,
declared in `Translation/MLIRToTarget.h`.

`MQSSCompiler::getSupportedInputFormats()` lists OpenQASM 2 alongside the Quake and Catalyst
formats; see [Discovering Supported Input Formats](library.md#discovering-supported-input-formats).

## Caveats and Limitations

The importer covers a small, fixed subset of OpenQASM 2. Keep the following in mind.

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
- **Round trips do not reproduce the input text.** Translating to Quake and exporting back with
  `--mlir-to-qasm2` yields a normalized program: the registers are always named `q` and `c`, and
  gates may be rewritten by the passes in between. Compare circuits by behavior, not by text.

### Detection and diagnostics

- **`MQSSCompiler` recognizes QASM2 by its header.** Input is treated as OpenQASM only if its first
  token, after whitespace and `//` comments, is `OPENQASM`. A QASM file without the header is parsed
  as MLIR and fails with an MLIR parse error. `mqss-translate --import-qasm2` does not require the
  header, since you have already said the input is QASM.
- **Only the version prefix is checked.** `OPENQASM 2.0;` and `2.x` headers pass; anything else,
  such as `OPENQASM 3.0;`, is rejected. OpenQASM 3 syntax cannot be imported, although it can be
  exported (see above).
- **The first error stops translation.** The importer reports one diagnostic and gives up, so fixing
  it may reveal the next one. Through `MQSSCompiler` it is followed by a generic "failed to parse
  input circuit" message.
- **Locations are line and column within the text.** Diagnostics point to `<qasm2>:line:col`, not to
  the file path, even when the text came from a file passed to `compile`.
- **`InputFormat::QASM2` is for reporting.** It makes `getSupportedInputFormats()` list OpenQASM 2,
  but it is not an option you set: the format is always detected from the content.

## Tests

The translations are covered by:

- lit tests under `tests/dialects`, which pipe `mqss-opt` into `mqss-translate`, run as part of
  `check-mqss`: for example `GHZToIQMQASM.mlir` (QASM3), `PLANQCTranspilationToQASM2.qke`,
  `IQMTranspileToIQMJSON.qke`, the `*ToQIR*` tests and `CatalystIQMDecomposition.mlir` (QIR for
  Catalyst input). The `MLIRToQASM*.mlir` tests in `tests/dialects/catalyst-quantum` pin the
  adjoint, controlled-rotation and constant-folding behavior and each rejection listed above.
  `QASM2ToQuakeBell.test` covers the importer;
- unit tests in `tests/unittests/MQSSCIInterfaces/test_Interface.cpp`, which compile the Bell and
  GHZ fixtures in Quake, OpenQASM 2 and Catalyst-quantum form to OpenQASM 2 and 3 through
  `compileSource` and check the rejected inputs. Run them with `make test-interfaces`.
