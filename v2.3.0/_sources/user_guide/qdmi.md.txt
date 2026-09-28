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

# QDMI in MQSS Quantum Compilation Suite

The
[Quantum Device Management Interface](https://munich-quantum-software-stack.github.io/QDMI/v1.3.2/)
(QDMI) interface provides a standard interface layer for communicating with back-end quantum
hardware. Various compiler passes (especially for transpilation) need information about the
target-device to efficiently map the input quantum circuit to the topology of the target-device.
Currently, the Qubit Mapping pass (`--CommonMappingPass`) uses the QDMI client interface to query a
target device for the device coupling map. In the future, we plan to leverage the QDMI interface to
drive more optimization passes.
