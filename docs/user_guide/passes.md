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
  OpenQASM etc.).

Before using the passes, please take note of the following:

1. Passes prefixed with `Common` operate on both the `Quake` and `Catalyst-quantum` MLIR dialects.
   This lets a single implementation serve multiple front-end SDKs without duplicating the
   optimization logic per dialect.
2. Transpilation — that is native-gate-set mapping and basis conversion — is currently only enabled
   for the `Quake` dialect.
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

### CommonDecompositionPass

Performs gate decomposition: `{Cx}` → `HCzH`, `{Cz}` → `HCxH`, `{H}` → `Rz-X-Rz`, or `{CH}` →
`S-H-T-CNOT-TAdj-H-SAdj`.

Pass Options:

- `--mode=<string>` — Select pass mode: `CxToHCzH`, `CzToHCxH`, `HToRzXRz`, or `CHToCX`

Example Invocation:

- `--CommonDecompositionPass=mode=CxToHCzH`

Note: This is a representative dialect-agnostic decomposition pass. It will be superseded by the
`BasisConversionPass` in the future, which performs recursive, device-aware decomposition rather
than a fixed set of rewrites.

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
- `--qdmi=<QDMI Device Name>` - Query QDMI Device for Coupling Map (Needs Device .so file).

Example invocation:

- `--CommonMappingPass=qdmi=cxx_qdmi.conf`</br> where `cxx_qdmi.conf` contains the path to the qdmi
  device shared object file and the device name prefix. See `tests/dialects/quake/cxx_qdmi.conf` for
  more details.

### BasisConversionPass

This pass decomposes all gate operations in the input MLIR dialect into the native gate set of the
target quantum device. It incorporates numerous decomposition patterns and operates recursively,
repeatedly rewriting non-native gates until every operation belongs to the requested native set (or
no further decomposition rule applies).

Note: Currently only available for the `Quake` MLIR dialect.

Pass Options:

- `gates=<comma-separated list of gates>`

Example Invocation:

- `--BasisConversionPass=gates=rx,cz,rz`

#### Supported Gate Mnemonics

The `gates` option (and the corresponding `native_gate_set` argument of `MQSSCompiler::compile`, see
[Choosing a Backend](library.md#choosing-a-backend)) takes a comma-separated list of gate mnemonics.
These are the mnemonics the pass recognizes, both as members of the requested native set and as
gates it knows how to decompose:

| Mnemonic    | Gate                            | Quake Operation   | Notes                                                                     |
| ----------- | ------------------------------- | ----------------- | ------------------------------------------------------------------------- |
| `h`         | Hadamard                        | `quake.h`         | Uncontrolled, single target.                                              |
| `x`         | Pauli-X                         | `quake.x`         | Uncontrolled, single target.                                              |
| `cx`        | Controlled-X (CNOT)             | `quake.x`         | Exactly one control.                                                      |
| `y`         | Pauli-Y                         | `quake.y`         | Uncontrolled, non-adjoint.                                                |
| `cy`        | Controlled-Y                    | `quake.y`         | Exactly one control, non-adjoint.                                         |
| `z`         | Pauli-Z                         | `quake.z`         | Uncontrolled, single target.                                              |
| `cz`        | Controlled-Z                    | `quake.z`         | Exactly one control.                                                      |
| `s`         | S (√Z phase gate)               | `quake.s`         | Non-adjoint.                                                              |
| `sdg`       | S† (adjoint of S)               | `quake.s`         | Adjoint form.                                                             |
| `t`         | T (⁴√Z phase gate)              | `quake.t`         | Non-adjoint.                                                              |
| `tdg`       | T† (adjoint of T)               | `quake.t`         | Adjoint form.                                                             |
| `r1`        | R1(θ) phase rotation            | `quake.r1`        | One parameter, non-adjoint, uncontrolled.                                 |
| `rx`        | Rx(θ)                           | `quake.rx`        | One parameter, non-adjoint, uncontrolled.                                 |
| `sx`        | √X — fixed π/2 rotation about X | `quake.rx`        | Recognized only when the parameter is the constant `π/2`; see note below. |
| `crx`       | Controlled-Rx(θ)                | `quake.rx`        | Exactly one control, one parameter.                                       |
| `ry`        | Ry(θ)                           | `quake.ry`        | One parameter, uncontrolled.                                              |
| `cry`       | Controlled-Ry(θ)                | `quake.ry`        | Exactly one control, one parameter.                                       |
| `rz`        | Rz(θ)                           | `quake.rz`        | One parameter, non-adjoint, uncontrolled.                                 |
| `crz`       | Controlled-Rz(θ)                | `quake.rz`        | Exactly one control, one parameter.                                       |
| `u2`        | U2(φ, λ)                        | `quake.u2`        | Two parameters, uncontrolled.                                             |
| `u3`        | U3(θ, φ, λ)                     | `quake.u3`        | Three parameters, uncontrolled.                                           |
| `swap`      | SWAP                            | `quake.swap`      | Two targets, uncontrolled.                                                |
| `phased_rx` | PhasedRx(θ, φ)                  | `quake.phased_rx` | Two parameters, non-adjoint, uncontrolled.                                |

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

## CodeGen Passes

These passes lower the optimized and transpiled MLIR down toward a target transport format,
ultimately producing QIR or OpenQASM.

### lower-quake-to-qir

The MQSS Quake-to-QIR conversion pass pipeline.

Pass Options:

- `profile=<string>` - Target transport layer format or QIR-Profile, <name[:version]>. Valid names:
  `qir`, `qir-base`, `qir-adaptive`, `qir-full`. version: `2.0`, `2.1`. [Default: `qir-base:2.0`]

Example Invocation:

- `--lower-quake-to-qir=profile=qir-base:2.0`

### quake-to-qasm2

Transforms a Quake MLIR module into OpenQASM 2. Invocation:

- `--quake-to-qasm2`

### convert-quantum-to-llvm

Performs a dialect conversion from the Catalyst-quantum dialect to the LLVM dialect.

Pass Options:

- `--use-array-backed-registers=<bool>` — Use the array-backed-registers conversion pattern for
  `quantum.insert` ops. [Default: `false`]

Invocation:

- `--convert-quantum-to-llvm`</br>

Note: This pass emits the LLVM MLIR dialect and **not** LLVM IR. To emit LLVM IR, this pass should
be followed by the `mlir-to-llvmIR` pass.

### mlir-to-llvmIR

Transforms the LLVM dialect into LLVM IR.

- `--mlir-to-llvmIR`

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
