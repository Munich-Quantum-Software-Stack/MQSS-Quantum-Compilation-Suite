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

#include "Passes/CodeGen/CodeGenPasses.h"
#include "Passes/Transforms/Dialects.h"
#include "Passes/Transforms/Pipelines.h"
#include "Passes/Transforms/TransformPasses.h"
#include "Passes/Verification/Instrumentation.h"
#include "mlir/IR/Dialect.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include "llvm/Support/Process.h"
#include "llvm/Support/ToolOutputFile.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/Location.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Parser/Parser.h>
#include <mlir/Pass/PassRegistry.h>
#include <mlir/Transforms/Passes.h>

using namespace llvm;

struct ToQirPipelineOptions
    : public mlir::PassPipelineOptions<ToQirPipelineOptions> {
  Option<std::string> profile{
      *this, "profile",
      llvm::cl::desc("Target transport layer format, <name:version>. Valid "
                     "names: \"qir\", \"qir-base\", \"qir-adaptive\", "
                     "\"qir-full\". version: \"2.0\", "
                     "\"2.1\", [Default: \"qir-base:2.0\"]"),
      llvm::cl::init("qir-base:2.0")};
};

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;

  mqss::mqssci::opt::registerMQSSDialects(registry);

  // All passes and pipelines must be registered with MLIR's global pass
  // registry *before* the PassPipelineCLParser below is constructed, since
  // that parser scans the registry at construction time to build its own
  // CLI flags (per-pass flags and pipeline names).
  registerMQSSTransformsPasses();
  registerMQSSCodeGenPasses();

  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return mlir::createCanonicalizerPass();
  });
  mlir::registerPass(
      []() -> std::unique_ptr<mlir::Pass> { return mlir::createCSEPass(); });

  mlir::registerPassPipeline(
      "O1", "MQSS-O1 optimization pipeline",
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::mqssci::opt::O1(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {});
  mlir::registerPassPipeline(
      "O2", "MQSS-O2 optimization pipeline",
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::mqssci::opt::O2(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {});
  mlir::registerPassPipeline(
      "O3", "MQSS-O3 optimization pipeline",
      [](mlir::OpPassManager &pm, StringRef options,
         std::function<LogicalResult(const Twine &)> errorHandler) {
        mqss::mqssci::opt::O3(pm);
        return mlir::success();
      },
      [](llvm::function_ref<void(const mlir::detail::PassOptions &)>) {});

  mlir::PassPipelineRegistration<ToQirPipelineOptions>(
      "lower-quake-to-qir", "MQSS Quake to QIR Conversion pipeline",
      [](mlir::OpPassManager &pm, const ToQirPipelineOptions &opts) {
        auto convertto = processQIRLoweringOpts(opts.profile);
        mqss::mqssci::opt::QIRConversionPipeline(pm, convertto);
      });

  // Must be constructed after every pass/pipeline above is registered, and
  // before CLI parsing happens below (registerAndParseCLIOptions), since
  // LLVM's cl::opt system requires every option to exist before
  // cl::ParseCommandLineOptions runs.
  cl::opt<bool> VerificationModeCLOpt(
      "mqssci-verify",
      cl::desc(
          "Perform Circuit Equivalence Check after each pass, default:false"),
      cl::init(0));

  llvm::StringRef toolName = "MQSS Optimizer\n";

  // Register and parse command line options.
  auto [inputFilename, outputFilename] =
      registerAndParseCLIOptions(argc, argv, toolName, registry);

  auto verifymode = VerificationModeCLOpt.getValue();
  // Built from CL options only after parsing has happened, so it reflects
  // whatever flags the user actually passed.
  MlirOptMainConfig config = MlirOptMainConfig::createFromCLOptions();

  // When reading from stdin and the input is a tty, it is often a user
  // mistake and the process "appears to be stuck". Print a message to let the
  // user know about it!
  if (inputFilename == "-" &&
      sys::Process::FileDescriptorIsDisplayed(fileno(stdin)))
    llvm::errs() << "(processing input from stdin now, hit ctrl-c/ctrl-d to "
                    "interrupt)\n";

  // Set up the input file.
  std::string errorMessage;
  auto file = mlir::openInputFile(inputFilename, &errorMessage);
  if (!file) {
    llvm::errs() << errorMessage << "\n";
    return mlir::asMainReturnCode(failure());
  }

  auto output = mlir::openOutputFile(outputFilename, &errorMessage);
  if (!output) {
    llvm::errs() << errorMessage << "\n";
    return mlir::asMainReturnCode(failure());
  }

  // Creating a copy of MlirOptMainConfig so as to avoid creating
  // a separate PassPipelineCLParser.
  auto defaultConfig = config;
  config.setPassPipelineSetupFn(
      [defaultConfig,
       verifymode](mlir::PassManager &pm) -> mlir::LogicalResult {
        auto errorHandler = [&](const llvm::Twine &msg) {
          mlir::emitError(mlir::UnknownLoc::get(pm.getContext())) << msg;
          return mlir::failure();
        };
        // Apply whatever the user asked for on the CLI: a single pass, a
        // named pipeline (O1/O2/O3/lower-quake-to-qir), or an explicit
        // -pass-pipeline=... string.
        if (failed(defaultConfig.setupPassPipeline(pm))) {
          return mlir::failure();
        }
        llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy> snapshot;
        if (verifymode) {
          pm.addInstrumentation(
              std::make_unique<mqss::mqssci::verify::VerifyPassInstrumentation>(
                  std::move(snapshot)));
          return mlir::success();
        }
        return mlir::success();
      });

  return mlir::asMainReturnCode(
      MlirOptMain(output->os(), std::move(file), registry, config));
}
