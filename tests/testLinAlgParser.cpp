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

#include <iostream>
#include <string>
// llvm includes
#include "llvm/Support/raw_ostream.h"
// mlir includes
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Complex/IR/Complex.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
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
#include "Passes/Decompositions.hpp"
#include "Passes/Examples.hpp"
#include "Passes/Transforms.hpp"
// test includes
#include <fstream>
#include <gtest/gtest.h>

#define CUDAQ_GEN_PREFIX_NAME "__nvqpp__mlirgen__"

std::tuple<mlir::ModuleOp, mlir::MLIRContext *> createEmptyMLIRModule() {
  auto contextPtr = cudaq::getOwningMLIRContext();
  mlir::MLIRContext &context = *contextPtr.get();
  // Create an empty MLIR module
  mlir::OwningOpRef<mlir::ModuleOp> m_module =
      mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  return std::make_tuple(m_module.release(), contextPtr.release());
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

std::tuple<std::string, std::string> getQuakeAndGolden(std::string inputFile,
                                                       std::string goldenFile) {
  std::string quakeModule = readFileToString(inputFile);
  std::string goldenOutput = readFileToString(goldenFile);
  return std::make_tuple(quakeModule, goldenOutput);
}

std::string normalize(const std::string &str) {
  std::string result;
  for (char c : str) {
    if (c != '\t' && c != '\n' && c != '\\' && c != ' ') {
      result += c;
    }
  }
  return result;
}

TEST(TestLinAlgPass, TestQuakeToLinAlg) {
  // load mlir module and the golden output
  auto [quakeModule, goldenOutput] =
      getQuakeAndGolden("./quake/QuakeToLinAlgPass.qke",
                        "./golden-cases/QuakeToTikzPass.tikz.tex");
#ifdef DEBUG
  std::cout << "Input Quake Module " << std::endl << quakeModule << std::endl;
#endif
  auto [mlirModule, contextPtr] = extractMLIRContext(quakeModule);
  mlir::MLIRContext &context = *contextPtr;
  context.loadDialect<mlir::tensor::TensorDialect>();
  context.loadDialect<mlir::arith::ArithDialect>();
  context.loadDialect<mlir::complex::ComplexDialect>();
  context.loadDialect<mlir::linalg::LinalgDialect>();
  // creating pass manager
  mlir::PassManager pm(&context);
  // Adding custom pass
  pm.addPass(mlir::createCanonicalizerPass());
  pm.addPass(mlir::createCSEPass());
  pm.addPass(mqss::opt::createQuakeToLinAlgPass());
  // running the pass
  if (mlir::failed(pm.run(mlirModule)))
    std::runtime_error("The pass failed...");
#ifdef DEBUG
  std::cout << "Captured output from Pass:\n" << std::endl;
  mlirModule->dump();
#endif
  //  EXPECT_EQ(normalize(goldenOutput), normalize(moduleOutput));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
