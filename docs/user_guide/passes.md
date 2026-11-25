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

# Passes

The MLIR passes available within the MQSS Quantum Compilation Suite are described here. Passes are
grouped into three categories:

- target-agnostic optimization passes that clean up and simplify a circuit regardless of the
  intended hardware,
- target-specific transpilation passes that adapt a circuit to a concrete device, and
- code-generation passes that lower the MLIR dialect down toward an executable representation (QIR,
  OpenQASM etc.). These write text rather than MLIR, so they are exposed through `mqss-translate`
  instead of `mqss-opt`; see [Code Generation Passes](#code-generation-passes).

Before using the passes, please take note of the following:

1. Passes prefixed with `Common` operate on both the `Quake` and `Catalyst-quantum` MLIR dialects.
   This lets a single implementation serve multiple front-end SDKs without duplicating the
   optimization logic per dialect.
2. Transpilation — that is native-gate-set mapping and basis conversion — works on both the `Quake`
   and the `Catalyst-quantum` dialect.
3. Refer to the [Example Usage](#example-usage) section below for instructions on how to enable and
   invoke a pass.

## Standard Optimization Passes (Target Device Agnostic)

These passes rewrite the circuit into an equivalent but simpler or more canonical form. Because they
do not depend on any device characteristics, they can be run at any stage of the pipeline and in any
combination.

### CommonCommutePass

This pass searches for and commutes gates that match a specific pattern. Commuting gates past one
another does not change the circuit's semantics, but it can expose further optimization
opportunities (for example, bringing two cancellable gates adjacent to each other).

Pass Options:

- `--mode=<string>` — Select pattern to commute: `CX-RX`, `RX-CX`, `CX-X`, `X-CX`, `CX-Z`, `Z-CX`

Example Invocation:

- `--CommonCommutePass=mode=CX-RX`

### CommonGateCancellationPass

Performs cancellation of gates that follow a specific pattern. The behavior depends on `mode`:

- `CancelGate` looks for gate operations of the same type and cancels them when they act on the same
  qubit operands (for example, two adjacent `CNOT`s on the same control and target, which together
  form the identity). Supported gate types: `CNOT`, `PauliX`, `PauliZ`, `PauliY`, and `Hadamard`.
- `CancelNullRotation` removes `RX`, `RY`, or `RZ` gates whose rotation angle is a constant multiple
  of 2π (i.e. an effective no-op rotation), regardless of adjacency to another gate.

Pass Options:

- `--mode=<string>` — Select pattern to cancel: `CancelGate` or `CancelNullRotation`

Invocation:

- `--CommonGateCancellationPass=mode=CancelGate`

### CommonNormalizeArgAnglePass

Normalizes the angle argument of the rotation gates `RX`, `RY`, and `RZ` — for instance, wrapping
angles into a canonical range so that equivalent rotations are represented identically. This makes
downstream cancellation and folding more effective.

Invocation:

- `--CommonNormalizeArgAnglePass`

### CommonReductionPass

Performs circuit reduction: `H-Z-H` → `X`, `H-X-H` → `Z`, `SAdj-Z` → `S`, or `S-Z` → `SAdj`.

Pass Options:

- `--mode=<string>` — Select pass mode: `HXHToZ`, `HZHToX`, `SAdjZToS`, or `SZToSAdj`

Example Invocation:

- `--CommonReductionPass=mode=HXHToZ`

Note: Here `H` (or `Hadamard`) refers to the Hadamard gate operation, `S` to the phase gate, and
`SAdj` to its adjoint.

### CommonSwitchPass

Commutes and switches gates. The pass first runs the `CommonCommutePass` and then replaces a
specified gate operation, effectively reordering a gate sequence into a preferred canonical form.

Pass Options:

- `--mode=<string>` — Select pattern to switch: `XYZHtoHXYZ` or `HXYZtoXYZH`

Example Invocation:

- `--CommonSwitchPass=mode=HXYZtoXYZH`

### CommonCNOTReversePass

Reverse the control and targets of each CNot gate in a circuit.

Invocation:

- `--CommonCNOTReversePass`

### canonicalize

Canonicalizes dialect operations. This is the standard MLIR canonicalization pass, which applies the
canonicalization patterns registered by each operation to fold constants and normalize the IR.

Invocation:

- `--canonicalize`

### cse

Eliminates common sub-expressions, removing redundant computations that produce the same value.

Invocation:

- `--cse`

## Transpilation Passes (Target Device Specific)

These passes adapt a circuit to a specific quantum device by respecting its connectivity and native
gate set. Unlike the optimization passes above, their output depends on the target hardware
description supplied to the pass.

### CommonMappingPass

A dialect-agnostic qubit mapping pass. It maps logical (algorithmic) qubits to physical (device)
qubits, inserting the operations needed to satisfy the target device's connectivity constraints. The
target's coupling map can be supplied either as a JSON file or queried directly from a QDMI device.

Pass Options:

- `--input=<string>` — Path to JSON input (Coupling Map of target device)
- `--qdmi=<path to .conf file>` — Query the coupling map from a QDMI device.

Example invocation:

- `--CommonMappingPass=qdmi=tests/input/cxx_qdmi.conf`</br> run from the repository root. The
  `.conf` file contains one line with the path to the QDMI device shared object file and the device
  prefix. See `tests/input/cxx_qdmi.conf` and `tests/input/mqt_qdmi.conf` for examples.

When neither option is given, the pass maps onto a built-in default device (currently 5 qubits), so
the allocated qubit register is widened accordingly.

For `Catalyst-quantum`, whose qubits have value semantics (every gate returns a new SSA value for
each qubit), the pass threads the latest value of each qubit through the mapped gates and
measurements. Every gate therefore stays connected to the measurements that depend on it, so a later
`canonicalize` cannot delete it as dead code, and each returned measurement bit comes from its own
measurement.

### BasisConversionPass

This pass decomposes all gate operations in the input MLIR dialect into the native gate set of the
target quantum device. It incorporates numerous decomposition patterns and operates recursively,
repeatedly rewriting non-native gates until every operation belongs to the requested native set (or
no further decomposition rule applies).

Note: Works on both the `Quake` and the `Catalyst-quantum` MLIR dialects. For `Catalyst-quantum` the
pass threads each qubit's value through the gates it creates, so the decomposed circuit keeps the
data dependencies of the original. See [Catalyst-quantum Gate Names](#catalyst-quantum-gate-names)
for the gates the Catalyst front end recognizes.

Pass Options:

- `gates=<comma-separated list of gates>` — The native gate set to decompose to. Defaults to
  `h,cx,rz`.
- `qdmi=<path to .conf file>` — Query the native gate set from a QDMI device instead of using
  `gates`. The `.conf` file has the same format as for [`CommonMappingPass`](#commonmappingpass).
- `device=<device name>` — Select a QDMI device by the name it reports, when the session has more
  than one. Defaults to the first device. If no device matches, the error message lists the
  available names. Wrap names containing spaces in braces or quotes. For example, the mock device in
  `tests/input/cxx_qdmi.conf` is named `C++ Device with 5 qubits`, selected with
  `device={C++ Device with 5 qubits}`.

Example Invocation:

- `--BasisConversionPass=gates=rx,cz,rz`
- `--BasisConversionPass=qdmi=tests/input/cxx_qdmi.conf`, run from the repository root
- `--BasisConversionPass="qdmi=tests/input/cxx_qdmi.conf device=<device name>"`

When `qdmi` is set:

- The device's operations are queried once per pass run. Each operation name is used as a gate
  mnemonic as is, so it must match the [Supported Gate Mnemonics](#supported-gate-mnemonics) table
  below. For example, IQM devices report `prx`. Names the pass does not recognize have no effect.
- An operation whose number of qubits or parameters does not match the table is ignored with a
  warning.
- If `gates` is also given, the QDMI gate set is used and a warning is emitted.
- The pass fails with an error if the device cannot be loaded, no device matches `device`, or every
  reported operation was ignored (or none was reported).

#### Supported Gate Mnemonics

The `gates` option (and the corresponding `native_gates` argument of `MQSSCompiler::compile`, see
[Choosing a Native-Gate Set and Backend](library.md#choosing-a-native-gate-set-and-backend)) takes a
comma-separated list of gate mnemonics. These are the mnemonics the pass recognizes, both as members
of the requested native set and as gates it knows how to decompose:

| Mnemonic | Gate                            | Quake Operation   | Notes                                                                     |
| -------- | ------------------------------- | ----------------- | ------------------------------------------------------------------------- |
| `h`      | Hadamard                        | `quake.h`         | Uncontrolled, single target.                                              |
| `x`      | Pauli-X                         | `quake.x`         | Uncontrolled, single target.                                              |
| `cx`     | Controlled-X (CNOT)             | `quake.x`         | Exactly one control.                                                      |
| `y`      | Pauli-Y                         | `quake.y`         | Uncontrolled, non-adjoint.                                                |
| `cy`     | Controlled-Y                    | `quake.y`         | Exactly one control, non-adjoint.                                         |
| `z`      | Pauli-Z                         | `quake.z`         | Uncontrolled, single target.                                              |
| `cz`     | Controlled-Z                    | `quake.z`         | Exactly one control.                                                      |
| `s`      | S (√Z phase gate)               | `quake.s`         | Non-adjoint.                                                              |
| `sdg`    | S† (adjoint of S)               | `quake.s`         | Adjoint form.                                                             |
| `t`      | T (⁴√Z phase gate)              | `quake.t`         | Non-adjoint.                                                              |
| `tdg`    | T† (adjoint of T)               | `quake.t`         | Adjoint form.                                                             |
| `r1`     | R1(θ) phase rotation            | `quake.r1`        | One parameter, non-adjoint, uncontrolled.                                 |
| `rx`     | Rx(θ)                           | `quake.rx`        | One parameter, non-adjoint, uncontrolled.                                 |
| `sx`     | √X — fixed π/2 rotation about X | `quake.rx`        | Recognized only when the parameter is the constant `π/2`; see note below. |
| `crx`    | Controlled-Rx(θ)                | `quake.rx`        | Exactly one control, one parameter.                                       |
| `ry`     | Ry(θ)                           | `quake.ry`        | One parameter, uncontrolled.                                              |
| `cry`    | Controlled-Ry(θ)                | `quake.ry`        | Exactly one control, one parameter.                                       |
| `rz`     | Rz(θ)                           | `quake.rz`        | One parameter, non-adjoint, uncontrolled.                                 |
| `crz`    | Controlled-Rz(θ)                | `quake.rz`        | Exactly one control, one parameter.                                       |
| `u2`     | U2(φ, λ)                        | `quake.u2`        | Two parameters, uncontrolled.                                             |
| `u3`     | U3(θ, φ, λ)                     | `quake.u3`        | Three parameters, uncontrolled.                                           |
| `swap`   | SWAP                            | `quake.swap`      | Two targets, uncontrolled.                                                |
| `prx`    | PhasedRx(θ, φ)                  | `quake.phased_rx` | Two parameters, non-adjoint, uncontrolled.                                |

Notes:

- `sx` is not a distinct Quake operation — it is the special case of `quake.rx` whose angle is the
  compile-time constant π/2. An `rx` with any other (or non-constant) angle is classified as plain
  `rx`, not `sx`. Because any device with a generic `rx` can trivially perform its π/2 special case,
  requesting `rx` in the native set also satisfies `sx` without listing it explicitly.
- Gates outside this table (e.g. adjoint `rx`/`ry`/`rz`/`r1`, or operations like `quake.measure`)
  are not recognized by this pass and are left untouched.
- If a gate present in the circuit cannot be legalized into the requested native set — no chain of
  decomposition rules bottoms out in only native mnemonics — the pass emits a warning and leaves
  that gate as-is rather than looping forever.

#### Catalyst-quantum Gate Names

In the `Catalyst-quantum` dialect gates are `quantum.custom` operations identified by name. These
are the names the Catalyst front end recognizes for each mnemonic:

| Mnemonic | `quantum.custom` name            |
| -------- | -------------------------------- |
| `h`      | `Hadamard` (or `H`)              |
| `x`      | `PauliX`                         |
| `cx`     | `CNOT`                           |
| `y`      | `PauliY`                         |
| `cy`     | `CY`                             |
| `z`      | `PauliZ`                         |
| `cz`     | `CZ`                             |
| `s`      | `S`                              |
| `sdg`    | `S` with the `adjoint` attribute |
| `t`      | `T`                              |
| `tdg`    | `T` with the `adjoint` attribute |
| `rx`     | `RX`                             |
| `crx`    | `CRX`                            |
| `ry`     | `RY`                             |
| `cry`    | `CRY`                            |
| `rz`     | `RZ`                             |
| `crz`    | `CRZ`                            |
| `u2`     | `U2`                             |
| `u3`     | `U3`                             |
| `swap`   | `SWAP`                           |
| `prx`    | `PhasedRx`                       |

The `r1` and `sx` mnemonics are Quake-only for now: the Catalyst front end does not recognize `R1`
or `SX` yet, and a `quantum.custom` with one of those names is rejected when the module is analyzed.

## Code Generation Passes

These passes lower the optimized and transpiled MLIR toward a target format. Except for
`expand-measurements`, they write text (or a different dialect) instead of rewriting the circuit, so
they are not `mqss-opt` flags. Each one is exposed as a translation of `mqss-translate`, which runs
the pass, or a pipeline built around it, and streams the result to its output. See
[Translating Circuits with mqss-translate](mqss-translate.md) for usage and for what each one
produces.

| Pass (factory in `CodeGenPasses.h`)                 | `mqss-translate` flag                                              |
| --------------------------------------------------- | ------------------------------------------------------------------ |
| `MLIRToQASMPass` (`mlir-to-qasm`)                   | `--mlir-to-qasm2`, `--mlir-to-qasm3`                               |
| `QuakeDialectToIQMJSONPass` (`quake-to-iqmJSON`)    | `--mlir-to-iqm-json`                                               |
| `QuantumConversionPass` (`convert-quantum-to-llvm`) | none; used by `--mlir-to-qir*` for Catalyst input                  |
| `LLVMDialectToLLVMIRPass` (`mlir-to-llvmIR`)        | `--llvm-dialect-to-llvm-ir`, and the last step of `--mlir-to-qir*` |

`MLIRToQASMPass` emits OpenQASM 2 or 3 for a Quake or Catalyst-quantum module. It takes the option
`mode=qasm2|qasm3`; from C++, `MLIRToQASMPass(os, qasm3)` writes to a stream. It builds an MQT Core
`QuantumComputation` and prints it, and it fails with a diagnostic on anything OpenQASM cannot
express (dynamic angles, loops, unsupported operations); see
[What is rejected](mqss-translate.md#what-is-rejected). `QuakeToQASM2Pass` is CUDA-Q's own OpenQASM
2 emitter for Quake. It stays in the library but neither command-line tool exposes it.

`QuantumConversionPass` converts the Catalyst-quantum dialect to the LLVM dialect (MLIR, not LLVM
IR). It has the option `use-array-backed-registers`, which defaults to `false`. The QIR translations
run it for Catalyst input, followed by `LLVMDialectToLLVMIRPass`, which turns the LLVM dialect into
LLVM IR.

### expand-measurements

Expands a `quake.mz` that measures a whole qubit register (`quake.mz %veq : (!quake.veq<N>) -> ...`)
into one `quake.mz` per qubit.

Invocation:

- `--expand-measurements`

Note: Some lowering paths have no concept of "measure this whole register in one instruction" — IQM
JSON in particular, where every `"measure"` instruction names exactly one qubit. Without this pass,
lowering a whole-register measurement to such a format can silently drop every qubit but one, or (if
the same op is otherwise duplicated elsewhere in the pipeline) produce multiple measurements that
collide on the same key — IQM's own circuit validator rejects that outright. Every `!quake.veq<N>`
this pass encounters must have a statically-known size (true for any circuit produced by this
pipeline), so it unrolls directly into `N` individual `quake.extract_ref`/`quake.mz` pairs at
rewrite time — no runtime loop is introduced. A `quake.mz` that's already per-qubit is left
untouched; any unsupported shape (a dynamically-sized register, or a consumer other than
`quake.discriminate`) fails the pass with a diagnostic rather than silently mistranslating it.

See `tests/unittests/input/multi_rotation_circuit.qke` (used by
`MQSSCIInterfacesTest.CompilesToIQMJSONForWholeRegisterMeasurement` in `test_Interface.cpp`) for a
circuit that measures a whole register and the resulting IQM JSON, one measurement per qubit.

The IQM JSON emitter (`QuakeDialectToIQMJSONPass`) expects IQM's native gate set (`prx`, `cz`) and
per-qubit measurements. `mqss-translate --mlir-to-iqm-json` runs `expand-measurements` for you, so
the usual flow is:

```sh
mqss-opt test.qke --BasisConversionPass=gates=prx,cz --cse --canonicalize \
  | mqss-translate --mlir-to-iqm-json
```

See `tests/dialects/quake/IQMTranspileToIQMJSON.qke` for a full example, including the expected JSON
output.

## Pass Pipelines

Pass pipelines bundle several passes together under a single flag, providing preset optimization
levels analogous to a compiler's `-O` flags.

Note: The pass pipelines are under active development and their exact composition may change.

### --O1

The MQSS-O1 optimization pipeline.</br> Passes enabled:

- `cse`
- `canonicalize`

### --O2

MQSS-O2 optimization pipeline</br> Passes enabled:

- `CommonGateCancellationPass`
- `CommonCNOTReversePass`
- `CommonCommutePass`
- `cse`
- `canonicalize`

### --O3

MQSS-O3 optimization pipeline</br> Passes enabled:

- `cse`
- `canonicalize`

### Output pipelines (C++ only)

The output stage that `mqss-translate` and `MQSSCompiler` run is built from pipelines in the
`mqss::mqssci::pipeline` namespace, declared in `Passes/Pipelines/Pipelines.h`. They are not
`mqss-opt` flags, since the passes at their end write text:

- `QASMLoweringPipeline(pm, qasm3, os)` appends `cse`, `canonicalize` and then `MLIRToQASMPass`. The
  cleanup folds trivial loops and constant angle expressions before the circuit is printed.
- `QIRConversionPipeline(pm, "<profile>:<version>", os)` lowers a Quake module to QIR with CUDA-Q's
  conversion pipeline and writes LLVM IR. The profile is one of `qir`, `qir-base`, `qir-adaptive` or
  `qir-full`, and the version is `2.0` or `2.1`.
- `CatalystQIRConversionPipeline(pm, os)` lowers a Catalyst-quantum module to QIR. It runs
  `QuantumConversionPass`, then `LLVMDialectToLLVMIRPass`, and emits the `__catalyst__qis__*`
  runtime calls. It does not decompose gates, so run `BasisConversionPass` first if the circuit
  needs it.

## Verifying a Pipeline's Correctness

Any pass or pipeline above can be checked for correctness with the `--mqssci-verify` flag, which
confirms that a transformation didn't change what the circuit computes. See
[Verifying Circuit Correctness](verification.md) for details.

## Example Usage

### Using mqss-opt

`mqss-opt` operates directly on an MLIR file, applying the passes specified on the command-line in
order.

**1. Quake**

```sh
mqss-opt test.qke --cse --canonicalize --BasisConversionPass=gates=rx,cz,rz
```

**2. Catalyst-quantum**

```sh
mqss-opt test.mlir --CommonMappingPass=input=/workspaces/MQSS-Quantum-Compilation-Suite/tests/input/qmap.json
```

`mqss-opt` writes MLIR. To turn the result into OpenQASM, IQM JSON or QIR, pipe it into
`mqss-translate`, for example
`mqss-opt test.qke --BasisConversionPass=gates=rx,cz,rz | mqss-translate --mlir-to-qasm2`. See
[Translating Circuits with mqss-translate](mqss-translate.md).

Note: Check the directory `tests/dialects` for more test cases using `mqss-opt` and example pass
invocations.

### Using mqss-cc

`mqss-cc` is a wrapper script that takes `C++`/`Python` source code as input, converts the source to
the appropriate MLIR dialect, and then runs `mqss-opt` on that dialect. It is the convenient entry
point when you want to start from kernel source rather than from an existing MLIR file.

Note: Currently, the script checks the extension of the source `.cpp` or `.py` and then performs the
appropriate translation. If a `.cpp` is detected, it is assumed that the source is a cudaq kernel.
If the source is a `.py` then it is assumed to be a `catalyst` kernel.

**1. For cudaq-quake**

```bash
mqss-cc test.cpp --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

**2. For catalyst-quantum**

```bash
mqss-cc test.py --function circuit --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

Note: Check the directory `tests/code` for more test cases using `mqss-cc` and example pass
invocations.
