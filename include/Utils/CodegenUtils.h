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

#include "cudaq/Optimizer/CodeGen/Passes.h"
#include "mlir/Pass/PassManager.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <string>

using namespace mlir;

// Some devices report their native gates under a different name than this
// pass's own mnemonic for the same physical gate -- e.g. IQM's own
// documentation describes its native single-qubit gate as "arbitrary X and
// Y rotations," reported by at least one device as "r", which is exactly
// this pass's "prx" (R(theta, phi) = exp(-i*theta/2*(cos(phi)*X +
// sin(phi)*Y))). Translate known aliases to this pass's own mnemonic before
// anything else treats the string as one, so a device (or a --gates=
// string) using "r" is understood the same way as "prx".
// Shared, single source of truth for device-name -> canonical mnemonic.
inline const llvm::StringMap<llvm::StringRef> &getMnemonicAliasTable() {
  static const llvm::StringMap<llvm::StringRef> aliases = {
      {"r", "prx"},
  };
  return aliases;
}

inline const llvm::StringRef qasmGateDefinitionBody(llvm::StringRef mnemonic) {
  static const llvm::StringMap<llvm::StringRef> qasmgateDef = {
      {"phased_rx",
       "(param0,param1) q0 { U(param0,-pi/2 + param1,pi/2 - param1) q0; }"}};
  auto it = qasmgateDef.find(mnemonic);
  return it == qasmgateDef.end() ? "" : it->second;
}

// BasisConversionPass's own mnemonic -> the literal gate name
// cudaq::translateToOpenQASM writes for it. Only needed for mnemonics
// whose QASM2 spelling differs from the mnemonic string itself.
inline llvm::StringRef mnemonicToQasmGateName(llvm::StringRef mnemonic) {
  static const llvm::StringMap<llvm::StringRef> qasmNames = {
      {"prx", "phased_rx"},
  };
  auto it = qasmNames.find(mnemonic);
  return it == qasmNames.end() ? mnemonic : it->second;
}

inline llvm::StringRef resolveMnemonicAlias(llvm::StringRef gate) {
  const auto &aliases = getMnemonicAliasTable();
  auto it = aliases.find(gate);
  return it == aliases.end() ? gate : it->second;
}

static void addQIRConversionPipeline(mlir::OpPassManager &pm,
                                     llvm::StringRef convertTo) {
  auto convertFields = convertTo.split(':');
  if (convertFields.first == "qir" || convertFields.first == "qir-full") {
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "full:" +
                                                   convertFields.second.str());
  } else if (convertFields.first == "qir-base") {
    pm.addNestedPass<mlir::func::FuncOp>(cudaq::opt::createDelayMeasurements());
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "base-profile:" +
                                                   convertFields.second.str());
  } else {
    cudaq::opt::addConvertToQIRAPIPipeline(pm, "adaptive-profile:" +
                                                   convertFields.second.str());
  }
}

// Processes the options passed to the Quake to QIR lowering
// pass. The options should be of the form "profile-name:version".
// Where, profile name could be "qir", "qir-base", "qir-adaptive",
// "qir-full". The version could be "2.0" or "2.1". E.g. qir-base:2.0.
// pure: no side effects, no aborts — just parse-or-fail
static std::optional<std::string>
tryProcessQIRLoweringOpts(const std::string &opts) {
  llvm::SmallVector<llvm::StringRef, 2> parts;
  llvm::StringRef(opts).split(parts, ':', /*MaxSplit=*/1);

  llvm::StringRef name = "";

  if (!parts.empty()) {
    name = parts[0];
  } else {
    return std::nullopt;
  }
  llvm::StringRef version = parts.size() > 1 ? parts[1] : "2.0";

  // Validate profile
  if (name != "qir" && name != "qir-base" && name != "qir-adaptive" &&
      name != "qir-full") {
    return std::nullopt;
  }
  // Validate version
  if (version != "2.0" && version != "2.1") {
    return std::nullopt;
  }

  return (name + ":" + version).str();
}

static std::string processQIRLoweringOpts(std::string opts) {
  if (auto result = tryProcessQIRLoweringOpts(opts))
    return *result;
  llvm::report_fatal_error("Invalid QIR lowering options: '" + Twine(opts) +
                           "'");
}
