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
#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"
#include "Passes/Transforms/Transforms.h"
#include "mlir/IR/Dialect.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/Location.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Parser/Parser.h>
#include <mlir/Pass/PassManager.h>
#include <mlir/Pass/PassRegistry.h>
#include <mlir/Transforms/Passes.h>

using namespace llvm;

struct ToQirPipelineOptions
    : public mlir::PassPipelineOptions<ToQirPipelineOptions> {
  Option<std::string> profile{
      *this, "profile",
      llvm::cl::desc(
          "Target transport layer format, <name[:version[:suboptions]]>. Valid "
          "names: \"qir\", \"base\", \"adaptive\", \"full\". version: \"2.0\", "
          "\"2.1\", [Default: \"qir:2.0\"]"),
      llvm::cl::init("base:2.0")};
};

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;

  mqss::opt::registerMQSSDialects(registry);

  // Register Dialect Agnostic Passes
  registerMQSSTransformsPasses();

  // Register CodeGen Passes
  registerMQSSCodeGenPasses();

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCanonicalizerPass();
  });
  mlir::registerPass(
      []() -> std::unique_ptr<mlir::Pass> { return mlir::createCSEPass(); });

  // Register Pass-pipelines
  mlir::registerPassPipeline(
      "O1",                            // pipeline name (used on CLI too)
      "MQSS-O1 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::opt::O1(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );
  mlir::registerPassPipeline(
      "O2",                            // pipeline name (used on CLI too)
      "MQSS-O2 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::opt::O2(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );
  mlir::registerPassPipeline(
      "O3",                            // pipeline name (used on CLI too)
      "MQSS-O3 optimization pipeline", // description
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::opt::O3(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {}
      // options callback
  );

  mlir::PassPipelineRegistration<ToQirPipelineOptions>(
      "lower-quake-to-qir", "MQSS Quake to QIR Conversion pipeline",
      [](mlir::OpPassManager &pm, const ToQirPipelineOptions &opts) {
        // Split "name[:version[:suboptions]]" on ':'
        llvm::SmallVector<llvm::StringRef, 3> parts;
        llvm::StringRef(opts.profile).split(parts, ':', /*MaxSplit=*/2);

        llvm::StringRef name = parts.size() > 0 ? parts[0] : "qir";
        llvm::StringRef version = parts.size() > 1 ? parts[1] : "2.0";
        llvm::StringRef suboptions = parts.size() > 2 ? parts[2] : "";

        // Validate name
        if (name != "qir" && name != "base" && name != "adaptive" &&
            name != "full") {
          llvm::report_fatal_error("invalid QIR profile name '" + Twine(name) +
                                   "'; expected 'base', 'adaptive', or 'full'");
        }
        // Validate version
        if (version != "2.0" && version != "2.1") {
          llvm::report_fatal_error("invalid QIR version '" + Twine(version) +
                                   "'; expected '2.0' or '2.1'");
        }

        std::string separator = "-";
        if (name == "qir") {
          separator = "";
          name = "";
        }
        std::string convertto =
            ("qir" + separator + name + ":" + version).str();
        mqss::opt::QIRConversionPipeline(pm, convertto);
      });

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "MQSS Optimizer\n", registry));
}
