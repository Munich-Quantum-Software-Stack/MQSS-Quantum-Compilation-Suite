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
  This file contains the unitary tests for the transpiler mechanism.
  The shown target backends are: WMI, PlanQ, IQM and AQT

******************************************************************************/

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
// test includes
#include <fstream>
#include <gtest/gtest.h>

#define CUDAQ_GEN_PREFIX_NAME "__nvqpp__mlirgen__"

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

std::tuple<std::string, std::string> behaviouralTest(
    std::tuple<std::string, std::string, std::string, std::vector<std::string>>
        test) {
  std::string fileInputTest = std::get<1>(test);
  std::string fileGoldenCase = std::get<2>(test);
  std::vector<std::string> nativeGateSet = std::get<3>(test);
  // load mlir module and the golden output
  auto [quakeModule, goldenOutput] =
      getQuakeAndGolden(fileInputTest, fileGoldenCase);
#ifdef DEBUG
  std::cout << "Input Quake Module " << std::endl << quakeModule << std::endl;
#endif
  auto [mlirModule, contextPtr] = extractMLIRContext(quakeModule);
  mlir::MLIRContext &context = *contextPtr;
  // creating pass manager
  mlir::PassManager pm(&context);
  // std::string basis[] = {
  //       "h",  "s", "t", "rx", "ry",
  //       "rz", "x", "y", "z",  "x(1)", // TODO set to ms, gpi, gpi2
  //   };
  cudaq::opt::BasisConversionPassOptions options;
  options.basis = nativeGateSet;
  pm.addPass(createBasisConversionPass(options));
  // pass to canonical form and remove non-used operations
  pm.addPass(mlir::createCanonicalizerPass());
  pm.addPass(mlir::createCSEPass());
  // running the pass
  if (mlir::failed(pm.run(mlirModule)))
    std::runtime_error("The pass failed...");
#ifdef DEBUG
  std::cout << "Circuit after pass:\n";
  mlirModule->dump();
#endif
  // Convert the module to a string
  std::string moduleOutput;
  llvm::raw_string_ostream stringStream(moduleOutput);
  mlirModule->print(stringStream);
  return std::make_tuple(goldenOutput, moduleOutput);
}

class BehaviouralCudaqTranspiler
    : public ::testing::TestWithParam<std::tuple<
          std::string,             // name test
          std::string,             // input of the test
          std::string,             // expected output
          std::vector<std::string> // vector of Decomposition patterns
          >> {};

TEST_P(BehaviouralCudaqTranspiler, Run) {
  std::tuple<std::string,              // name test
             std::string,              // input of the test
             std::string,              // expected output
             std::vector<std::string>> // vector of Decomposition patterns
      p = GetParam();
  std::string testName = std::get<0>(p);
  SCOPED_TRACE(testName);
  auto [goldenOutput, moduleOutput] = behaviouralTest(p);
  EXPECT_EQ(goldenOutput, std::string(moduleOutput));
}

INSTANTIATE_TEST_SUITE_P(
    TranspilerTests, BehaviouralCudaqTranspiler,
    ::testing::Values(
        std::make_tuple("TestIQMTranspilation",
                        "./quake/cudaq-transpiler/TranspilerInput.qke",
                        "./golden-cases/cudaq-transpiler/IQMTranspilation.qke",
                        std::vector<std::string>{
                            "phased_rx", "z(1)"}), // IQM Native Gate Set
        // needed a decomposition of H
        std::make_tuple(
            "TestPlanQTranspilation",
            "./quake/cudaq-transpiler/TranspilerInput.qke",
            "./golden-cases/cudaq-transpiler/PlanQTranspilation.qke",
            std::vector<std::string>{"h", "rx", "ry", "rz", "x(1)",
                                     "z(1)"}), // PlanQ Native Gate Set*/
        // MS is missing
        std::make_tuple("TestAQTTranspilation",
                        "./quake/cudaq-transpiler/TranspilerInput.qke",
                        "./golden-cases/cudaq-transpiler/AQTTranspilation.qke",
                        std::vector<std::string>{
                            "x", "y", "z", "h", "s", "t", "rx", "ry", "rz",
                            "x(1)", "z(1)", "swap"}), // AQT Native Gate Set
        std::make_tuple("TestWMITranspilation",
                        "./quake/cudaq-transpiler/TranspilerInput.qke",
                        "./golden-cases/cudaq-transpiler/WMITranspilation.qke",
                        std::vector<std::string>{
                            "rx", "ry", "rz", "h", "phased_rx", "phased_ry",
                            "phased_rz", "x(1)", "z(1)"}) // WMI Native Gate Set
        ),
    [](const ::testing::TestParamInfo<BehaviouralCudaqTranspiler::ParamType>
           &info) {
      // Use the first element of the tuple (testName) as the custom test name
      return std::get<0>(info.param);
    });

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
