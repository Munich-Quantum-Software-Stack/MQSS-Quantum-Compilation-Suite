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

# MQSS FAQs

## What is MQSS?

**MQSS** stands for _Munich Quantum Software Stack_, which is a project of the _Munich Quantum
Valley (MQV)_ initiative and is jointly developed by the _Leibniz Supercomputing Centre (LRZ)_ and
the Chairs for _Design Automation (CDA)_ and for _Computer Architecture and Parallel Systems (CAPS)_
at TUM. It provides a comprehensive compilation and runtime infrastructure for on-premise and remote
quantum devices, supports modern compilation and optimization techniques, and enables current and
future high-level abstractions for quantum programming.

This stack is designed to deploy in various scenarios via flexible configuration options, including
stand-alone scenarios for individual systems, cloud access to multiple devices, and tight
integration into HPC environments supporting quantum acceleration.

A concrete instance of the MQSS is deployed at the LRZ for the MQV, serving as a single access point
to all of its quantum devices via multiple compatible access paths, including a web portal, command
line access via web credentials as well as the option for hybrid access with tight integration with
LRZ's HPC systems. It facilitates the connection between end-users and quantum computing platforms
by its integration within HPC infrastructures, such as those found at the LRZ.

## Under which license is this collection of passes released?

This collection of MLIR passes is released under the Apache License v2.0 with LLVM Exceptions. See
[LICENSE](https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE) for more
information. Any contribution to the project is assumed to be under the same license.
