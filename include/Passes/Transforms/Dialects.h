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

#pragma once

#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"

namespace mqss::mqssci::opt {

// Registers every dialect and dialect-translation the MQSS passes operate on
// or lower into (Quake, Catalyst-quantum, standard MLIR dialects, LLVM
// translation). Call this before constructing an mlir::MLIRContext.
void registerMQSSDialects(mlir::DialectRegistry &registry);

// Convenience: builds a DialectRegistry via registerMQSSDialects() and
// returns a ready-to-use MLIRContext with all dialects loaded.
std::unique_ptr<mlir::MLIRContext> createMQSSContext();

} // namespace mqss::mqssci::opt
