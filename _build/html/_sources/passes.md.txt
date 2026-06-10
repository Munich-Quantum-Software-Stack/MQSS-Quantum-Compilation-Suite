# Passes

Following are the MLIR passes available within the MQSS Quantum Compilation Suite.

Notes:

1. The passes Prefixed ```Common``` operate on both the ```Quake``` and ```Catalyst-quantum```
MLIR dialects.
2. Transpilation or Native-gate-set mapping is currently only enabled for ```Quake```.
3. Please refer to the example usage section below on instructions for how to enable a pass

## Standard Optimization Passes

### CommonCommutePass

This pass searches commutes gates that match a specific pattern.

- `--mode=<string>` — Select pattern to commute: `CX-RX`, `RX-CX`, `CX-X`, `X-CX`, `CX-Z`, `Z-CX`

### CommonDecompositionPass

Perform gate decomposition: `{Cx}` → `HCzH`, `{Cz}` → `HCxH`, or `ReverseCx`.

- `--mode=<string>` — Select pass mode: `CxToHCzH`, `CzToHCxH`, or `ReverseCx`

### CommonGateCancellationPass

Performs cancellation of gates following a specific pattern.

- `--mode=<string>` — Select pattern to cancel: `CancelGate` or `CancelNullRotation`

### CommonNormalizeArgAnglePass

Normalize arg angle of RX, RY, RZ gates.

### CommonReductionPass

Perform circuit reduction: `{HZH}` → `X`, `{HXH}` → `Z`, `{SAdjZ}` → `S`, or `{SZ}` → `SAdj`.

- `--mode=<string>` — Select pass mode: `HXHToZ`, `HZHToX`, `SAdjZToS`, or `SZToSAdj`

### CommonSwitchPass

Commute and switch gates.

- `--mode=<string>` — Select pattern to switch: `XYZHtoHXYZ` or `HXYZtoXYZH`

### canonicalize

Canonicalize operations.

- `--disable-patterns=<string>` — Labels of patterns to filter out
- `--enable-patterns=<string>` — Labels of patterns to use (all others filtered out)
- `--max-iterations=<long>` — Max iterations between applying patterns / simplifying regions
- `--max-num-rewrites=<long>` — Max number of pattern rewrites within an iteration
- `--region-simplify` — Perform control flow optimizations on the region tree
- `--test-convergence` — Fail pass on non-convergence to detect cyclic patterns
- `--top-down` — Seed the worklist in top-down order

### cse

Eliminate common sub-expressions.

## Transpilation passes

### CommonMappingPass

A dialect-agnostic Qubit mapping pass. Maps logical qubits to physical qubits.

- `--input=<string>` — Path to JSON input

### fermioniq-gate-set-mapping

Convert Quake kernels to Fermioniq gate set.</br>
The fermioniq backend supports the following gates:</br>
`"h",  "s", "t", "rx", "ry", "rz", "x", "y", "z",  "x(1)"`.

### ionq-gate-set-mapping

Convert Quake kernels to IONQ gate set.</br>
The IONQ backend supports the following gates:</br>
`"h",  "s", "t", "rx", "ry", "rz", "x", "y", "z",  "x(1)"`.

### iqm-gate-set-mapping

Convert Quake kernels to IQM gate set.</br>
The IQM backend supports the following gates:</br>
`"phased_rx", "z(1)"`.

### oqc-gate-set-mapping

Convert Quake kernels to OQC gate set.</br>
The OQC backend supports the following gates:</br>
`"h", "s", "t", "r1", "rx", "ry", "rz", "x", "y", "z", "x(1)"`

### qci-gate-set-mapping

Convert Quake kernels to QCI gate set.</br>
The QCI backend supports the following gates:</br>
`"h", "s", "t", "rx", "ry", "rz", "x", "y", "z", "x(1)"`

### quantinuum-gate-set-mapping

Convert Quake kernels to Quantinuum gate set.</br>
The Quantinuum backend supports the following gates:</br>
`"h", "s", "t", "rx", "ry", "rz", "x", "y", "z", "x(1)",`.

### print-quake-gates-pass

Traverses a given MLIR module, prints its gates and a description of the operands of each gate.

### print-catalyst-gates-pass

Traverses a given MLIR module, prints its gates and a description of the operands of each gate.

---

## Example Usage

**1. For cudaq-quake**

```bash
mqss-cc test.cpp --emit-qir --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

**2. For catalyst-quantum**

```bash
mqss-cc test.py --function circuit --stage QuantumCompilationStage --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```
