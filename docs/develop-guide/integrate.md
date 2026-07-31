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

set(CUDAQ_AUTO_FETCH ON CACHE BOOL "" FORCE)
set(CATALYST_AUTO_FETCH ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(mqssci
  GIT_REPOSITORY https://github.com/akshay9594/MQSS-Passes-Suite.git
  GIT_TAG        8aa079e27d50cd90abc0e2322b5362b0138d7b79
)

FetchContent_MakeAvailable(mqssci)
```

Then within the appropriate `CMakeLists.txt`:

```cmake
find_package(MQSSCI REQUIRED).   # If the name of file is FindMQSSCI.cmake
```
