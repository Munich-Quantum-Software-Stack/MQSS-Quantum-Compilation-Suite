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

# Verification Infrastructure

This page describes the implementation of circuit-equivalence verification: what it checks, how it's
wired into `mqss-opt`, and what to know before extending it. For the end-user-facing `--verify`
flag, see [Verifying Circuit Correctness](../user_guide/verification.md).

## Design: instrumentation, not a pass

Verification is implemented as an `mlir::PassInstrumentation`, not as an ordinary MLIR pass. This
was a deliberate choice: a pass only runs when a user explicitly places it in a `-pass-pipeline=...`
string or invokes it by name, which means the user has to remember to bracket whatever subset of
passes they run with a "snapshot" pass and a "verify" pass. `PassInstrumentation` instead attaches
to the `mlir::PassManager` itself and its `runBeforePass`/`runAfterPass` hooks fire around _every_
pass the manager executes — one pass, a whole pipeline, or an arbitrary `-pass-pipeline=...` string
all get the same coverage automatically, with no special-casing required from the user.

The class is `mqss::mqssci::verify::VerifyPassInstrumentation`:

- Declared in `include/Passes/Verification/Instrumentation.h`.
- Defined in `lib/Passes/Verification/EquivalenceVerification.cpp`.
- Built into the `MQSSCIVerificationPasses` static library
  (`lib/Passes/Verification/CMakeLists.txt`), which depends on `MQT::CoreIR` and `MQT::QCEC` in
  addition to `MQSSSupportedDialects`, `MLIRPass`, and `MLIRTransforms`.

## How a check runs

1. **`runBeforePass(pass, op)`** — `op` is the `ModuleOp` about to be processed. A
   `DialectAnalysisSelector` walks it and, for each kernel `FuncOp`, `createMQTQuantumComputation`
   translates its gate/measurement operations into an `qc::QuantumComputation` (MQT Core's circuit
   representation). This snapshot is cached in `cached_module_snapshot`, an
   `llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy>` keyed by **kernel symbol name**,
   not by `FuncOp` identity. This matters: a pass may legitimately replace a `FuncOp` with a new
   object (signature conversion, cloning) while keeping its symbol name — keying by name is what
   lets the "before" and "after" snapshots still be matched up correctly in that case.
2. **`runAfterPass(pass, op)`** — re-runs the same translation to populate the "after" half of the
   snapshot, then runs an equivalence check per kernel using `mqt-qcec`'s
   `ec::EquivalenceCheckingManager`.

## Equivalence checking configuration

`runAfterPass` constructs a default-initialized `ec::Configuration` and sets
`config.functionality.checkPartialEquivalence = true`. A default `Configuration` already enables a
_portfolio_ of checkers, not a single hand-picked one:

```cpp
bool runConstructionChecker = false;
bool runSimulationChecker = true;
bool runAlternatingChecker = true;
bool runZXChecker = true;
```

On top of that, `EquivalenceCheckingManager`'s constructor performs one automatic fallback: if the
alternating (decision-diagram-based) checker is enabled but
`DDAlternatingChecker::canHandle(qc1, qc2)` returns false — typically because the circuits have
ancillary qubits it can't structurally represent — it disables the alternating checker and enables
the construction checker instead. The code in `runAfterPass` mirrors this same check explicitly
before calling `ecm.run()`, so it's applied even in scenarios where the manager's own internal check
might not catch it.

`ecm.equivalence()` returns an `ec::EquivalenceCriterion`, not a boolean. The values that represent
a "good" result are `Equivalent`, `EquivalentUpToGlobalPhase`, `EquivalentUpToPhase`, and
`ProbablyEquivalent` (the last only occurs when the simulation checker resolves things first, since
it is a probabilistic method — see
[mqt-qcec's equivalence checking documentation](https://mqt.readthedocs.io/projects/qcec/en/stable/equivalence_checking.html)
for what each checker actually verifies). Global-phase differences are expected and fine to accept:
gate decompositions frequently introduce a global phase relative to the original circuit, which has
no physical effect. When adding new success/failure handling, check the specific
`EquivalenceCriterion` value returned rather than testing for one specific variant.

## The `--verify` CLI flag

Declared in `mqss-cc.cpp` as a plain string option:

```cpp
cl::opt<string> VerificationModeCLOpt(
    "verify",
    cl::desc("Specify verification mode: off, once, after-each-pass"),
    cl::value_desc("mode"), cl::init("off"));
```

It's read out (`VerificationModeCLOpt.getValue()`) only _after_ `registerAndParseCLIOptions` has run
— reading a `cl::opt`'s value before parsing returns its unparsed default, not whatever the user
passed. The value is then captured into the lambda passed to
`MlirOptMainConfig::setPassPipelineSetupFn`, where `pm.addInstrumentation(...)` is conditionally
called only when the mode is `"once"` or `"after-each-pass"` — `"off"` skips adding the
instrumentation entirely, so it costs nothing at runtime rather than merely no-op'ing inside the
hooks.

Note: at present, `"once"` and `"after-each-pass"` are handled identically — the instrumentation
doesn't yet distinguish "check once at the end of the whole run" from "check after every pass".
Doing so requires overriding `PassInstrumentation::runBeforePipeline`/`runAfterPipeline` (which fire
once around the whole `OpPassManager` run rather than per-pass) so that the expensive `ecm.run()`
call only happens once for `"once"` mode. `runBeforePipeline`/`runAfterPipeline` don't hand you the
`Operation*` the way `runBeforePass`/`runAfterPass` do, but by the time `runAfterPipeline` fires the
snapshot map already has an up-to-date "after" half from the last `runAfterPass` call, so no
operation access is actually needed there.

### `setPassPipelineSetupFn` composes with, rather than replaces, the CLI's default pipeline handling

`registerAndParseCLIOptions`/`MlirOptMainConfig::createFromCLOptions()` already wire up a default
`passPipelineCallback` that knows how to apply whatever the user asked for on the CLI (a single
pass, a named pipeline, an explicit `-pass-pipeline=...` string). `setPassPipelineSetupFn`
**replaces** that callback outright rather than adding to it, so `mqss-cc.cpp` takes a copy of
`config` before installing its own callback (`auto defaultConfig = config;`), and the new callback
calls `defaultConfig.setupPassPipeline(pm)` first before adding the verification instrumentation.

## A CMake pitfall specific to this library

`lib/Passes/Verification/CMakeLists.txt` builds `MQSSCIVerificationPasses` as a static library (no
`SHARED` keyword). For a static `add_mlir_library`, the macro creates a separate `obj.<name>`
_object_ library that does the actual compiling; the `<name>` static target just archives
`$<TARGET_OBJECTS:obj.<name>>` and compiles nothing itself. Any `target_compile_options` or
`target_compile_definitions` call aimed at `MQSSCIVerificationPasses` directly (rather than
`obj.MQSSCIVerificationPasses`) silently has no effect on how `EquivalenceVerification.cpp` is
actually compiled. This bit both `-fexceptions` and `MQSS_ENABLE_DEBUG` during development — the fix
is the same each time:

```cmake
target_compile_options(MQSSCIVerificationPasses PRIVATE -fexceptions)
if(TARGET obj.MQSSCIVerificationPasses)
  target_compile_options(obj.MQSSCIVerificationPasses PRIVATE -fexceptions)
endif()
```

Apply the same `if(TARGET obj.<name>) ... endif()` guard for any future compile option or definition
added to this target.

## Testing

Verification is exercised via the same lit/FileCheck infrastructure described in
[Testing](develop-guide.md#testing). Two representative tests:
`tests/dialects/quake/IQMTranspileAndVerify.qke` and
`tests/dialects/quake/PLANQCTranspileAndVerify.qke`. Both follow the same pattern: run a real
transpilation pipeline with `--verify=after-each-pass` appended, and check for the printed result:

```sh
// RUN: %mqss-opt %s --BasisConversionPass=gates=phased_rx,cz --cse --canonicalize --verify=after-each-pass 2>&1 | FileCheck %s

// CHECK: Equivalence check Result for: __nvqpp__mlirgen__bellILm2EE is Equivalent
```

## Known limitations to keep in mind when extending this

- No distinction yet between `"once"` and `"after-each-pass"` (see above).
- Verification runs once per kernel `FuncOp` found by `DialectAnalysisSelector`; it does not
  currently skip kernels once they stop being translatable to `qc::QuantumComputation` (e.g. after
  lowering past the quantum dialects into LLVM dialect during code generation). Placing
  `--verify=after-each-pass` ahead of a `CodeGen` pass in a pipeline is untested territory.
- `createMQTQuantumComputation` silently skips any `Gate` enum value it has no translation case for
  (see `loadGates` in `include/Utils/MQTCoreUtils.h`) rather than erroring. A gate type added to the
  `Gate` enum in `include/Passes/Analysis/Extractor.h` without a corresponding case in `loadGates`
  will be dropped from the translated circuit rather than causing a build or runtime error. When
  adding a new gate type end-to-end, add its `loadGates` case in the same change.
