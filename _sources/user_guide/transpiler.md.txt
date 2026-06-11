# MQSS-Quantum Compilation Suite Transpiler

<!-- IMPORTANT: Keep the line above as the first line. -->
<!----------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-------------------------------------------------------------------------- -->

<!-- This file is a static page and included in the CMakeLists.txt file. -->

Note: Currently, transpilation is only possible for the Quake MLIR dialect
      using the native-gate set mapping passes via ```cudaq-opt```.
      Please refer to the [passes]() for details.

We are developing a MLIR Dialect Agnostic Transpiler for the MQSS Quantum
Compilation Suite. The transpiler will have two key components:

1. A list of Gate Decomposition patterns (e.g. Gate CNOT/Cx might decompose to
    Hadamard, CZ, Hadamard)
2. A (shortest-path / Dijkstra-style) graph based basis conversion algorithm which
   determines the least number of decomposition patterns required to map all gates
   in the source circuit (in an MLIR dialect) to the native-gate set of the target device.
