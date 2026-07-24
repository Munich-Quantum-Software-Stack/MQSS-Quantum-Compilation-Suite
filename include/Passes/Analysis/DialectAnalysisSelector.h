
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
*************************************************************************
  author Akshay Bhosale
  co-author: Claude AI Sonnet/Opus
  date   August 2026
  version 2.0.0
*************************************************************************/

#include "QuakeExtractor.h"
#include "CatalystExtractor.h"
#include <memory>
#include <string>

enum class QuantumDialect { Quake, Catalyst, Unknown };

static string DialectTytoString(QuantumDialect DialectTy) {

  switch (DialectTy) {
  case QuantumDialect::Quake:
    return "Quake";
  case QuantumDialect::Catalyst:
    return "Catalyst";
  default:
    return "Unknown";
  }
}
static QuantumDialect detectFramework(ModuleOp module) {
  QuantumDialect fw = QuantumDialect::Unknown;
  module.walk([&](Operation *op) {
    auto ns = op->getDialect()->getNamespace();
    if (ns == "quake") { fw = QuantumDialect::Quake; return WalkResult::interrupt(); }
    if (ns == "quantum") { fw = QuantumDialect::Catalyst; return WalkResult::interrupt(); }
    return WalkResult::advance();
  });
  return fw;
}

class DialectAnalysisSelector {
public:
  DialectAnalysisSelector(Operation *op) {
    auto module = cast<ModuleOp>(op);
    switch (detectFramework(module)) {
    case QuantumDialect::Quake:
      impl = std::make_unique<QuakeAnalysis>(module);
      DialectTy = QuantumDialect::Quake;
      break;
    case QuantumDialect::Catalyst:
      impl = std::make_unique<CatalystQuantumAnalysis>(module);
      DialectTy = QuantumDialect::Catalyst;
      break;
    default:
      impl = nullptr; // pass checks this
      DialectTy = QuantumDialect::Unknown;
    }
  }

  MyModuleAnalysis *get() { return impl.get(); }
  QuantumDialect getDialect() { return DialectTy; };

private:
  std::unique_ptr<MyModuleAnalysis> impl;
  QuantumDialect DialectTy;
};