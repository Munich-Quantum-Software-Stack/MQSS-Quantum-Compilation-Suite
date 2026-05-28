# Writing Passes for the MQSS

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

<!-- This file is a static page and included in the ./CMakeLists.txt file. -->

## Adding your pass

- Two types of passes can be added : Dialect-Agnostic and Dialect-specific
- The dialect agnostic passes should be added within ```lib/common/Passes```, whereas, dialect-specific
  passes should be added within ```lib/mqss-catalyst/lib/MQSSCatalystPasses``` (for catalyst-quantum)
  and ```lib/mqss-cudaq/lib/MQSSQuakePasses``` (for quake).
- A key step in adding your pass is pass registration. This ensures that the passes can be found by
  the targets ```mqss-cudaq-opt``` and ``````mqss-catalyst-opt```.
  1. To register a pass we need to make additions to 2 files ```Transforms.h``` and ```Transforms.td```.
     These files can be found within ```lib/mqss-catalyst/include/MQSSCatalystPasses``` and
     ```lib/mqss-cudaq/include/MQSSQuakePasses```.

  2. Here is an example of pass registration within ```lib/mqss-cudaq/include/MQSSQuakePasses/Transforms.td```:

  ```sh
    def CommonNormalizeArgAnglePass : Pass<"CommonNormalizeArgAnglePass", "mlir::ModuleOp"> {
      let summary = "Normalize Arg angle of RX, RY, RZ gates. ";

      let description = [{
        Normalize Arg angle of RX, RY, RZ gates.
      }];

      let constructor = "mqss_cudaq::opt::CommonNormalizeArgAnglePass()";
    }

  ```

    The pass is registered for the ```mqss-cudaq-opt``` target. Since the pass is dialect agnostic, the same pass can be
    registered for the ```mqss-catalyst-opt``` target by changing the namespace ```mqss_cudaq::opt```
    to ```mqss_catalyst::opt``` within the constructor.
  3. In ```Transforms.h``` within the ```mqss_cudaq::opt``` and ```mqss_catalyst::opt``` namespaces, the
     following should be added:

     ```sh
        std::unique_ptr<mlir::Pass> CommonNormalizeArgAnglePass();
     ```

## Pass Structure

## Adding a Dialect
