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

# Verifying Circuit Correctness

Every pass in this suite rewrites your quantum circuit in some way — decomposing gates, cancelling
redundant operations, mapping qubits to a device. These transformations are meant to preserve
exactly what your circuit does; only its representation should change. Verification is a built-in
safety net that checks this automatically: it compares the circuit before a transformation to the
circuit after, and tells you whether they are still equivalent.

You don't need to write any extra code to use it — it's a single flag you add to the `mqss-opt`
command.

## Why this matters

Compiler passes are ordinary code, and ordinary code can have bugs. A decomposition pass might use a
slightly wrong rotation angle; a mapping pass might drop a gate while rewiring qubits. Without
verification, a bug like this can silently produce a circuit that looks fine (it still parses and
compiles) but no longer computes the same thing. Verification catches this class of bug
automatically by checking equivalence rather than just checking that the output is well-formed.

## Methods Used

We use the Checking methods from the
[MQT-QCEC](https://mqt.readthedocs.io/projects/qcec/en/stable/equivalence_checking.html) library to
perform Equivalence Checking. Specifically, we use the following methods:

- Alternating Equivalence Checker (using Decision Diagrams)
- Simulation Equivalence Checker
- ZX-Calculus Equivalence

## Enabling verification

Add the `--mqssci-verify` flag to your `mqss-opt` invocation:

```sh
mqss-opt bell-state.qke --BasisConversionPass=gates=phased_rx,cz --mqssci-verify
```

It's a plain on/off switch — pass `--mqssci-verify` to enable it, or leave it out for the default
(off). When enabled, equivalence is checked after every pass in whatever you ran, so if you pass
several passes at once, each one gets its own check.

## Reading the output

When verification runs, it prints one line per quantum kernel (function) in your circuit, telling
you whether the before-and-after circuits are equivalent:

```text
[verify] __nvqpp__mlirgen__bellILm2EE: Equivalent
```

If a transformation changed what the circuit computes, this will instead say the circuit is **NOT
equivalent** — that's a signal that something in the pass pipeline you ran needs investigating. If a
pass itself fails to complete (unrelated to equivalence — a crash, an illegal rewrite, etc.),
verification is skipped for that pass and a message is printed instead explaining that the pass
didn't finish, so equivalence couldn't be checked.

## Where this fits

Verification works with any pass or pipeline you invoke on `mqss-opt` — a single pass, one of the
[preset pipelines](passes.md#pass-pipelines), or an explicit `-pass-pipeline=...` string. See
[Passes](passes.md) for the full list of passes you can combine it with, and
[Running test circuits](running.md) for more examples of invoking `mqss-opt`.

Note: `--mqssci-verify` is currently only available when invoking `mqss-opt` directly. The `mqss-cc`
front-end wrapper does not yet expose it as a first-class flag.
