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

# MQSS-Quantum Compilation Suite Transpiler

Transpilation involves two key components:

1. Qubit Mapping Mapping logical or algorithmic qubits to the physical qubits of target device
2. Basis Conversion (Native-gate set decomposition) Decomposing algorithmic gates to the native-gate
   set of target device

## Qubit Mapping

We have developed a Dialect-Agnostic qubit mapping pass for `quake` and `catalyst-quantum` dialects
that uses the [MQT-QMAP](https://mqt.readthedocs.io/projects/qmap/en/latest/) library for mapping
algorithmic qubits to the physical qubits of target device. Currently, only Super-conducting devices
are supported. The pass `CommonMappingPass` enables the mapping process. This pass requires as input
the `Coupling Map` of target device. The user can provide the Coupling Map in JSON format to the
pass (providing path to the JSON file is sufficient). An example is shown below:

```json
{
  "architecture": {
    "num_qubits": 5,
    "coupling_map": [
      [0, 1],
      [1, 0],
      [1, 2],
      [2, 1],
      [2, 3],
      [3, 2],
      [3, 4],
      [4, 3],
      [4, 0],
      [0, 4]
    ]
  },
  "mapper_settings": {
    "heuristic": "GateCountMaxDistance",
    "layering": "DisjointQubits",
    "initial_layout": "Identity",
    "pre_mapping_optimizations": false,
    "post_mapping_optimizations": false,
    "lookahead_heuristic": "None",
    "debug": false,
    "add_measurements_to_mapped_circuit": true
  }
}
```

In addition, users can specify the use of `QDMI` client interface for querying a QDMI device and
extracting the Coupling Map of the target device. The user needs to provide a `.conf` file with the
following information:

- Path to the QDMI device shared object (.so) file
- The device prefix

For e.g. for the CXX QDMI device, the information will be specified as:

```sh
/path/to/libcxx-qdmi-device.so CXX
```

Note: The QDMI library checks for the path of the `.conf` files to be a valid path. Currently, in
our testing, placing the `.conf` file within the same directory as the test input works best. Please
refer to the `RUN` command of the following test case for more details:
`tests/dialects/quake/QuakeQMapPass-02.qke`

The pass uses the Heuristic Mapping algorithm to perform the mapping on the MLIR dialect. For
details about Heuristic Mapping and the various fields within `mapper_settings` please refer to the
documentation : <https://mqt.readthedocs.io/projects/qmap/en/latest/mapping.html>.

## Basis Conversion (Native gate-set Decomposition)

The transpiler includes a Basis Conversion pass that can map the gate operations in the input MLIR
(quake) dialect to the native gate-set of the target device. The native-gate can be provided as a
comma separated list of gates to the pass `--BasisConversion=gates=phased_rx,cz`. The pass
recursively goes through each gate operation in the input dialect and maps the gate to one of the
native gates of the target device using a set of Decomposition Patterns. Please refer to
`include/Passes/transforms/DecompositionPatterns.h` for a list of decomposition patterns currently
available to the pass. If a the pass cannot decompose a certain gate operation in the input dialect
a warning message is displayed pointing to the failed decomposition.

We have tested the BasisConversion pass for decomposing to the native-gate set of target devices
provided by the following vendors:

```sh
IQM
PLANQC
WMI
```

For details on pass invocation and mapping to the devices of the aforementioned vendors, Please
refer to the [passes](passes.md) documentation and the following test cases:

```sh
tests/dialects/quake/IQMDecompositionToQIR.qke
tests/dialects/quake/PLANQCDecompositionToQASM2.qke
tests/dialects/quake/WMIDecompositionToQIR.qke
```
