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

#include "Translation/MLIRToTarget.h"
#include "Translation/QASM2ToQuake.h"
#include "mlir/Tools/mlir-translate/MlirTranslateMain.h"

// Translation counterpart of mqss-opt. It imports foreign circuit formats into
// MLIR and emits MLIR as a target format, e.g.
//   mqss-translate --import-qasm2 bell.qasm
//   mqss-translate --mlir-to-qasm3 circuit.mlir
// mlir-translate only runs translations (it never consults the pass
// registry), so every output format is registered as a translation.
int main(int argc, char **argv) {
  mqss::mqssci::translation::registerQASM2ToQuakeTranslation();
  mqss::mqssci::translation::registerMLIRToTargetTranslations();

  return mlir::failed(mlir::mlirTranslateMain(argc, argv, "MQSS Translator\n"));
}
