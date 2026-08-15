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

<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/_static/mqss_logo_dark.svg" width="20%">
    <img src="./docs/_static/mqss_logo.svg" width="20%">
  </picture>
</div>

# Munich Quantum Software Stack (MQSS)-Quantum Compilation Suite

 <a href="https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/actions/workflows/ci.yml">
  <img src="https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/actions/workflows/ci.yml/badge.svg?branch=develop" alt="CI" />
  </a>
  <a href="https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/actions/workflows/docs.yml">
  <img src="https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/actions/workflows/docs.yml/badge.svg?branch=develop" alt="Deploy Docs" />
  </a>

This repository contains a collection of compiler passes integrated into the MQSS to optimize,
transform, and lower quantum programs to instructions compliant with target Quantum devices. The
passes in this suite operate on quantum circuits represented using the state-of-the-art
[Multi-Level Intermediate Representation (MLIR)](https://mlir.llvm.org) framework.

The passes operate on either the quake MLIR dialect by Nvidia
[cudaq-quantum](https://github.com/NVIDIA/cuda-quantum) or catalyst-quantum dialect by
[pennlylane-catalyst](https://github.com/PennyLaneAI/catalyst).

<span style="color: red;">[NOTE] : This Suite is still under active development. You can expect
bugs.</span>

<div align="center">
  <a href="https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/">
  <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
</div>

## Key features

1. **Representation-Agnostic Optimizations**: Apply the same passes across multiple MLIR dialects.

2. **Cross-framework support**: Works with ecosystems like cudaq-quantum and Catalyst.

3. **Write once, reuse everywhere**: Shared optimization logic across compiler representations.

4. **Built to extend**: Add new passes without redesigning the framework.

5. **Native-IR interoperability**: Connect frameworks without replacing their IRs/dialects.

6. **Qubit Mapping**: A representation-agnostic logical/algorithmic to physical qubit mapping pass
   that uses [QDMI](https://github.com/Munich-Quantum-Software-Stack/QDMI) and
   [MQT-QMAP](https://github.com/munich-quantum-toolkit/qmap)

7. **Transpilation**: A transpilation pass to decompose to the native-gate set of target quantum
   devices (currently only for cudaq-quake).

Note: Please refer to the
[FAQs](https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/faqs/index.html)
section for more details on the passes, MLIR, MQSS and other related questions.

## Getting Started

To learn more about how to work with the MQSS Quantum Compilation Suite, please take a look at the
[Suite Documentation](https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/).
The page also contains
[installation instructions](https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/user_guide/build.html)
for officially released packages.

If you would like to write passes for the MQSS Quantum Compilation Suite or would like to integrate
the suite into your project follow the instructions in the
[Development Guide](https://munich-quantum-software-stack.github.io/MQSS-Quantum-Compilation-Suite/develop-guide/index.html).

## Contributing

There are many ways in which you can get involved with the MQSS Quantum Compilation Suite. If you
are interested in developing Passes, fixing bugs or adding additional features, this repository is a
great place to get started! For more information about contributing to the Suite, please take a look
at [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MQSS Quantum Compilation Suite is free and open source, released under the Apache License, Version
2.0. Refer to [LICENSE](LICENSE) for more information.

## Contact

The development of this project is led by the QCT department at the LRZ and the QSI department at
MQV gGmbH. You can also always reach us at <mqss@munich-quantum-valley.de>.

Please try to use the publicly accessible GitHub channels (issues, discussions, pull requests) to
allow for a transparent and open discussion as much as possible.
