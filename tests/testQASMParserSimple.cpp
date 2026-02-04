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
  date   December 2024
  version 1.0
  brief
  This file contains the unitary tests for each MLIR pass in the Munich Quantum
  Software Stack (MQSS).
  * Folder code has quantum kernels written in CudaQ (cpp).
  * Folder golden contains the expected modified quantum kernel in MLIR for each
    MLIR pass.
  1. In each test, the quantum kernel in CudaQ is lowered to QUAKE MLIR.
  2. Then the pass is applied to the QUAKE MLIR kernel.
  3. The output of the pass is compared to the expected output.
  4. Success if both expected output and the output obtained by the pass
matches.

******************************************************************************/

// QCEC checker headers
#include "EquivalenceCheckingManager.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "dd/DDDefinitions.hpp"
#include "ir/operations/Control.hpp"

#include <iostream>
#include <string>
// llvm includes
#include "llvm/Support/raw_ostream.h"
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
#include "common/RuntimeMLIR.h"
// includes mqss passes
#include "Passes/CodeGen.hpp"
// test includes
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include <zip.h>

#define CUDAQ_GEN_PREFIX_NAME "__nvqpp__mlirgen__"

std::string getEmptyQuakeKernel(const std::string kernelName,
                                std::string functionName) {
  std::string templateEmptyQuake =
      "module {"
      "  func.func @__nvqpp__mlirgen__KERNELNAME() attributes "
      "{\"cudaq-entrypoint\", \"cudaq-kernel\"} {"
      "   return"
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

std::tuple<std::unique_ptr<mlir::MLIRContext>,
           mlir::OwningOpRef<mlir::ModuleOp>>
extractMLIRContext(const std::string &quakeModule) {
  auto contextPtr = cudaq::getOwningMLIRContext();
  mlir::MLIRContext &context = *contextPtr.get();

  // Get the quake representation of the kernel
  auto quakeCode = quakeModule;
  auto m_module = mlir::parseSourceString<mlir::ModuleOp>(quakeCode, &context);
  if (!m_module)
    throw std::runtime_error("Module cannot be parsed");

  return std::make_tuple(std::move(contextPtr), std::move(m_module));
  //  return std::make_tuple(std::move(m_module), std::move(contextPtr));
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

std::string lowerQuakeCodeToOpenQASM(std::string quantumTask) {
  // auto [m_module, contextPtr] =
  //     extractMLIRContext(quantumTask);
  mlir::OwningOpRef<mlir::ModuleOp> m_module;
  std::unique_ptr<mlir::MLIRContext> contextPtr;
  std::tie(contextPtr, m_module) = extractMLIRContext(quantumTask);

  mlir::MLIRContext &context = *contextPtr;
  std::string postCodeGenPasses = "";
  bool printIR = false;
  bool enablePassStatistics = false;
  bool enablePrintMLIREachPass = false;

  auto translation = cudaq::getTranslation("qasm2");
  std::string codeStr;
  {
    llvm::raw_string_ostream outStr(codeStr);
    m_module->getContext()->disableMultithreading();
    if (mlir::failed(translation(m_module.get(), outStr, postCodeGenPasses,
                                 printIR, enablePrintMLIREachPass,
                                 enablePassStatistics)))
      throw std::runtime_error("Could not successfully translate to OpenQASM2");
  }
  // Regular expression to match the gate definition
  std::regex gatePattern(R"(gate\s+\S+\(param0\)\s*\{\n\})");
  // Remove the matching part from the string
  codeStr = std::regex_replace(codeStr, gatePattern, "");
  // m_module.release();
  // contextPtr.release();
  return codeStr;
}

std::vector<std::string> extractQASMFiles(const std::string &zipFilePath,
                                          const std::string &outputDir) {
  std::vector<std::string> qasmFiles;
  // Open the ZIP archive
  int err = 0;
  zip *archive = zip_open(zipFilePath.c_str(), ZIP_RDONLY, &err);
  if (!archive) {
    std::cerr << "Error opening ZIP file: " << zipFilePath << std::endl;
    return qasmFiles;
  }
  // Get number of files inside the ZIP
  zip_int64_t numEntries = zip_get_num_entries(archive, 0);
  for (zip_int64_t i = 0; i < numEntries; ++i) {
    const char *fileName = zip_get_name(archive, i, ZIP_FL_ENC_GUESS);
    if (!fileName)
      continue;
    std::string fileStr(fileName);
    if (fileStr.size() >= 5 && fileStr.substr(fileStr.size() - 5) == ".qasm") {
      // Extract the file
      zip_file *zFile = zip_fopen_index(archive, i, 0);
      if (!zFile) {
        std::cerr << "Error opening file inside ZIP: " << fileStr << std::endl;
        continue;
      }
      std::filesystem::path pathObj(fileStr);
      fileStr = pathObj.filename().string();
      std::string outputPath = outputDir + "/" + fileStr;
      std::ofstream outFile(outputPath, std::ios::binary);
      if (!outFile) {
        std::cerr << "Error creating output file: " << outputPath << std::endl;
        zip_fclose(zFile);
        continue;
      }
      // Read file content and write to disk
      char buffer[4096];
      zip_int64_t bytesRead;
      while ((bytesRead = zip_fread(zFile, buffer, sizeof(buffer))) > 0) {
        outFile.write(buffer, bytesRead);
      }
      zip_fclose(zFile);
      outFile.close();
      // Save extracted file name
      qasmFiles.push_back(outputPath);
      // std::cout << "Extracted: " << outputPath << std::endl;
    }
  }
  // Close the ZIP archive
  zip_close(archive);
  return qasmFiles;
}

TEST(TestMQSSPasses, TestQASMToQuake) {
  // Open the OpenQASM 3.0 file
  std::string goldenOutput =
      readFileToString("./golden-cases/test-parser-qasm.qke");
  std::ifstream inputQASMFile("./qasm/test-parser.qasm");
  std::string templateEmptyQuake =
      getEmptyQuakeKernel("ghz_indep_qiskit_10", "_ZN3ghzILm2EEclEv");
  // Read file content into a string
  std::stringstream buffer;
  buffer << inputQASMFile.rdbuf();
#ifdef DEBUG
  std::cout << "QASM input file:\n";
  std::cout << buffer.str() << "\n";
#endif
  // Convert to istringstream
  std::istringstream qasmStream(buffer.str());
  // creating empty mlir module
  mlir::OwningOpRef<mlir::ModuleOp> mlirModule;
  std::unique_ptr<mlir::MLIRContext> contextPtr;
  std::tie(contextPtr, mlirModule) = extractMLIRContext(templateEmptyQuake);
  mlirModule->getContext()->disableMultithreading();
  mlir::MLIRContext &context = *contextPtr;
#ifdef DEBUG
  std::cout << "Empty mlir module:\n";
  mlirModule->dump();
#endif
  // creating pass manager
  mlir::PassManager pm(&context);
  pm.nest<mlir::func::FuncOp>().addPass(
      mqss::opt::createQASM3ToQuakePass(qasmStream, false));
  // running the pass
  if (mlir::failed(pm.run(mlirModule.get())))
    std::runtime_error("The pass failed...");
#ifdef DEBUG
  std::cout << "Parsed Circuit from QASM:\n";
  mlirModule->dump();
#endif
  // Convert the module to a string
  std::string moduleOutput;
  llvm::raw_string_ostream stringStream(moduleOutput);
  mlirModule->print(stringStream);
  // Destroy the module
  // mlirModule.release();
  EXPECT_EQ(goldenOutput, std::string(moduleOutput));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
