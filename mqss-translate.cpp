/* This code and any associated documentation is provided "as is"

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
*/

#include "Translation/QASM2ToQuake.h"
#include "mlir/Tools/mlir-translate/MlirTranslateMain.h"

// Frontend counterpart of mqss-opt: translates foreign circuit formats into
// MLIR, e.g. `mqss-translate --import-qasm2 bell.qasm`.
int main(int argc, char **argv) {
  mqss::mqssci::translation::registerQASM2ToQuakeTranslation();
  return mlir::failed(mlir::mlirTranslateMain(argc, argv, "MQSS Translator\n"));
}
