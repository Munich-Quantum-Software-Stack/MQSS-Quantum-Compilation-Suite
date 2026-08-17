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

Performs gate decomposition: `{Cx}` → `HCzH`, `{Cz}` → `HCxH`, or `ReverseCx`.

Pass Options:

- `--mode=<string>` — Select pass mode: `CxToHCzH`, `CzToHCxH`, or `ReverseCx`

Example Invocation:

- `--CommonDecompositionPass=mode=CxToHCzH`

Note: This is a representative dialect-agnostic decomposition pass. It will be superseded by the
`BasisConversionPass` in the future, which performs recursive, device-aware decomposition rather
than a fixed set of rewrites.

### CommonGateCancellationPass

Performs cancellation of gates that follow a specific pattern. The pass looks for gate operations of
the same type and cancels them when they act on the same qubit operands (for example, two adjacent
`CNOT`s on the same control and target, which together form the identity). The supported gate
operation types are: `CNOT`, `PauliX`, `PauliZ`, `PauliY`, `Hadamard`, `RX`, `RY`, and `RZ`.

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

## CodeGen Passes

These passes lower the optimized and transpiled MLIR down toward a target transport format,
ultimately producing QIR or OpenQASM.

### lower-quake-to-qir

The MQSS Quake-to-QIR conversion pass pipeline.

Pass Options:

- `profile=<string>` - Target transport layer format or QIR-Profile, <name[:version[:suboptions]]>.
  Valid names: "qir", "base", "adaptive", "full". version: "2.0", "2.1", [Default: "qir:2.0"]

Example Invocation:

- `--lower-quake-to-qir=profile=base:2.0`

### quake-to-qasm2

Transforms a Quake MLIR module into OpenQASM 2. Invocation:

- `--quake-to-qasm2`

### convert-quantum-to-llvm

Performs a dialect conversion from the Catalyst-quantum dialect to the LLVM dialect.

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
- `cse`
- `canonicalize`
- `CommonCommutePass`

### --O3

MQSS-O3 optimization pipeline</br> Passes enabled:

- `cse`
- `canonicalize`

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
mqss-opt test.mlir --CommonMappingPass=input=/workspaces/MQSS-Passes-Suite/tests/input/qmap.json
```

Note: Check the directory `tests/dialects` for more test cases using `mqss-opt` and example pass
invocations.

### Using mqss-cc

``mqss-cc` is a wrapper script that takes `C++`/`Python` source code as input, converts the source
to the appropriate MLIR dialect, and then runs `mqss-opt` on that dialect. It is the convenient
entry point when you want to start from kernel source rather than from an existing MLIR file.

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
