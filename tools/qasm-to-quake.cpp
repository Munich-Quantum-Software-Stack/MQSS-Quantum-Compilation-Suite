/* This code and any associated documentation is provided "as is"

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
-------------------------------------------------------------------------
  author Martin Letras
  date   January 2025
  version 1.0
  brief
  This tool uses the QuakeToTikzPass to generate a latex file describing a given
input circuit

******************************************************************************/
// mlir includes
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR/Import.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h" // For translateModuleToLLVMIR
#include "mlir/Transforms/Passes.h"
// cudaq includes
#include "cudaq/Frontend/nvqpp/AttributeNames.h"
#include "cudaq/Optimizer/Transforms/Passes.h"
// includes in runtime
#include "Passes/CodeGen.hpp"
#include "common/RuntimeMLIR.h"

#include <boost/program_options.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

namespace po = boost::program_options;

std::string getEmptyQuakeKernel(const std::string kernelName,
                                std::string functionName) {
  std::string templateEmptyQuake =
      "module attributes {"
      "  llvm.data_layout = "
      "\"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-"
      "S128\", "
      "  llvm.triple = \"x86_64-unknown-linux-gnu\", "
      "  quake.mangled_name_map = {__nvqpp__mlirgen__KERNELNAME = "
      "\"FUNCTIONNAME\"}"
      "} {"
      "  func.func @__nvqpp__mlirgen__KERNELNAME() attributes "
      "{\"cudaq-entrypoint\", \"cudaq-kernel\"} {"
      "   return"
      "  }"
      ""
      "  func.func @FUNCTIONNAME(%arg0: !cc.ptr<i8>) {"
      "    return"
      "  }"
      "}";
  std::regex kernelNameRegex("KERNELNAME");
  std::regex functionNameRegex("FUNCTIONNAME");
  // Replace KERNELNAME and FUNCTIONNAME with the provided kernelName and
  // functionName
  templateEmptyQuake =
      std::regex_replace(templateEmptyQuake, kernelNameRegex, kernelName);
  templateEmptyQuake =
      std::regex_replace(templateEmptyQuake, functionNameRegex, functionName);
  return templateEmptyQuake;
}

std::tuple<mlir::ModuleOp, mlir::MLIRContext *>
extractMLIRContext(const std::string &quakeModule) {
  auto contextPtr = cudaq::getOwningMLIRContext();
  mlir::MLIRContext &context = *contextPtr.get();

  // Get the quake representation of the kernel
  auto quakeCode = quakeModule;
  auto m_module = mlir::parseSourceString<mlir::ModuleOp>(quakeCode, &context);
  if (!m_module)
    throw std::runtime_error("Module cannot be parsed");

  return std::make_tuple(m_module.release(), contextPtr.release());
}

std::string readFileToString(const std::string &filename) {
  std::ifstream file(filename); // Open the file
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return "";
  }
  std::ostringstream fileContents;
  fileContents << file.rdbuf(); // Read the whole file into the string stream
  return fileContents.str();    // Convert the string stream to a string
}

std::string lowerCppToQuake(std::string cppFile) {
  int retCode = std::system(("cudaq-quake " + cppFile + " -o ./o.qke").c_str());
  if (retCode)
    throw std::runtime_error("Quake transformation failed!!!");
  retCode = std::system(
      "cudaq-opt --canonicalize --unrolling-pipeline o.qke -o ./kernel.qke");
  if (retCode)
    throw std::runtime_error("Quake transformation failed!!!");
  // loading the generated mlir kernel of the given cpp
  std::string quakeModule = readFileToString("./kernel.qke");
  std::remove("./o.qke");
  std::remove("./kernel.qke");
  return quakeModule;
}

bool hasExtension(const std::string &filename,
                  const std::string &fileExtension) {
  // Check if the filename ends with .cpp
  if (filename.length() >= fileExtension.length()) {
    // Check for .cpp extension
    if (filename.substr(filename.length() - fileExtension.length()) ==
        fileExtension)
      return true;
  }
  return false;
}

int main(int argc, char *argv[]) {
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "input", po::value<std::string>(), "Input qasm file!. It must be .qasm")(
      "output", po::value<std::string>(),
      "The name of the output file. It must be *.qke");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  std::stringstream buffer;
  if (vm.count("input")) {
    if (!hasExtension(vm["input"].as<std::string>(), ".qasm")) {
      std::cout << "File " << vm["input"].as<std::string>()
                << " is not a valid supported file!" << std::endl;
      return 1;
    }
    // read the input file and stored into qasmProgram
    std::ifstream inputQASMFile(vm["input"].as<std::string>());
    buffer << inputQASMFile.rdbuf();
    std::cout << "Input file name " << vm["input"].as<std::string>()
              << std::endl;
  } else {
    std::cout << "Input file name was not set." << std::endl;
    return 1;
  }
  if (vm.count("output")) {
    if (!hasExtension(vm["output"].as<std::string>(), ".qke")) {
      std::cout << "Output file has not a correct qke extension!" << std::endl;
      return 1;
    }
    std::cout << "Output file name " << vm["output"].as<std::string>()
              << std::endl;
  } else {
    std::cout << "Output file name was not set." << std::endl;
    return 1;
  }
  setbuf(stdout, NULL);
  // loading qasm
  // Convert to istringstream
  std::istringstream qasmStream(buffer.str());
  std::filesystem::path pathObj(vm["input"].as<std::string>());
  std::string inputFileName = pathObj.filename().string();
  std::regex pattern(R"(^(.*?)[-_]*\.qasm$)");
  std::smatch match;
  if (!std::regex_match(inputFileName, match, pattern))
    throw std::runtime_error("Fatal error!");
  std::string kernelName = match[1];
  std::regex pattern2(R"([-_])");
  // Replace all occurrences of "-" and "_"
  kernelName = std::regex_replace(kernelName, pattern2, "");
  std::cout << "kernelName " << kernelName << std::endl;
  std::string templateEmptyQuake = getEmptyQuakeKernel(kernelName, "_function");
  // continue loading mlir module and context
  auto [mlirModule, contextPtr] = extractMLIRContext(templateEmptyQuake);
  mlir::MLIRContext &context = *contextPtr;
  mlirModule->getContext()->disableMultithreading();
  // creating pass manager
  mlir::PassManager pm(&context);
  // Adding custom pass
  pm.nest<mlir::func::FuncOp>().addPass(
      mqss::opt::createQASM3ToQuakePass(qasmStream));
  // running the pass
  if (mlir::failed(pm.run(mlirModule)))
    std::runtime_error("The pass failed...");
  // Convert the module to a string
  std::string moduleOutput;
  llvm::raw_string_ostream stringStream(moduleOutput);
  mlirModule->print(stringStream);

  // Open the file in output mode (create or overwrite)
  std::ofstream outFile(vm["output"].as<std::string>());
  // Check if the file was opened successfully
  if (outFile.is_open()) {
    // Write the content to the file
    outFile << moduleOutput;
    // Close the file
    outFile.close();
    std::cout << "Content successfully written to "
              << vm["output"].as<std::string>() << std::endl;
  } else
    std::cerr << "Failed to open" << vm["output"].as<std::string>()
              << " for writing" << std::endl;

  return 0;
}
