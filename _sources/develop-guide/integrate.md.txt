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

# Integrating the MQSS Quantum Compilation Suite within your project

One can integrate MQSS-Quantum Compilation Suite into their project as a cmake module using the
following entry:

```cmake

include(ExternalProject)

set(MQSSCI_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/mqssci-build)

ExternalProject_Add(mqssci
  GIT_REPOSITORY   https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite.git
  GIT_TAG           <commit-has/release tag>
  PREFIX            ${CMAKE_BINARY_DIR}/_deps/mqssci-build
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/_deps/mqssci-src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/_deps/mqssci-src   # in-source make, adjust if out-of-source supported
  CONFIGURE_COMMAND ""   # no configure step needed for plain make
  BUILD_COMMAND bash -c "make build"
  INSTALL_COMMAND   make target
  # BUILD_IN_SOURCE   TRUE  # set if module-Y's Makefile expects to run from its source root
)

set(MQSSCI_SRC_DIR ${CMAKE_BINARY_DIR}/_deps/mqssci-src)
```

Then within the appropriate `CMakeLists.txt`:

```cmake
find_package(MQSSCI REQUIRED).   # If the name of file is FindMQSSCI.cmake
```
