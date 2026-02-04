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
  4. Success if both expected output and the output obtained by the pass match.

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
// test includes
#include "Passes/CodeGen.hpp"
#include "Passes/Decompositions.hpp"
#include "Passes/Examples.hpp"
#include "Passes/Transforms.hpp"

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

std::string getQuake(std::string inputFile) {
  std::string quakeModule = readFileToString(inputFile);
  return quakeModule;
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

std::string lowerQuakeCodeToOpenQASM(std::string quantumTask) {
  auto [m_module, contextPtr] = extractMLIRContext(quantumTask);

  mlir::MLIRContext &context = *contextPtr;
  std::string postCodeGenPasses = "";
  bool printIR = false;
  bool enablePassStatistics = false;
  bool enablePrintMLIREachPass = false;

  auto translation = cudaq::getTranslation("qasm2");
  std::string codeStr;
  {
    llvm::raw_string_ostream outStr(codeStr);
    m_module.getContext()->disableMultithreading();
    if (mlir::failed(translation(m_module, outStr, postCodeGenPasses, printIR,
                                 enablePrintMLIREachPass,
                                 enablePassStatistics)))
      throw std::runtime_error("Could not successfully translate to OpenQASM2");
  }
  // Regular expression to match the gate definition
  std::regex gatePattern(R"(gate\s+\S+\(param0\)\s*\{\n\})");
  // Remove the matching part from the string
  codeStr = std::regex_replace(codeStr, gatePattern, "");
  return codeStr;
}

class EqualityTest : public testing::Test {
  void SetUp() override {
    qc1 = qc::QuantumComputation();
    qc2 = qc::QuantumComputation();

    config.execution.runSimulationChecker = false;
    config.execution.runAlternatingChecker = false;
    config.execution.runConstructionChecker = false;
    config.execution.runZXChecker = false;
  }

protected:
  // std::size_t nqubits = 1U;
  qc::QuantumComputation qc1;
  qc::QuantumComputation qc2;
  ec::Configuration config{};

  std::tuple<std::string, std::string, std::string,
             std::function<std::unique_ptr<mlir::Pass>()>>
      passInfo;
};

// TEST_F(EqualityTest, TestQuakeQMapPass01) {
//   std::string quakeModule =  getQuake("./quake/QuakeQMapPass-01.qke");
//   // get the QASM of the input module
//   std::string qasmInput = lowerQuakeCodeToOpenQASM(quakeModule);
//   #ifdef DEBUG
//     std::cout << "Input Quake Module:" << std::endl << quakeModule <<
//     std::endl; std::cout << "QASM input module:"<< std::endl << qasmInput <<
//     std::endl;
//   #endif
//   std::stringstream qasmStream = std::stringstream(qasmInput);
//   qc1.import(qasmStream, qc::Format::OpenQASM2);
//   auto [mlirModule, contextPtr] = extractMLIRContext(quakeModule);
//   mlir::MLIRContext &context = *contextPtr;
//   // creating pass manager
//   mlir::PassManager pm(&context);
//   // Defining test architecture
//   Architecture arch{};
//   /*
//       3
//       / \
//    4   2
//     |   |
//     0---1
//   */
//   const CouplingMap cm = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
//                           {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
//   arch.loadCouplingMap(5, cm);
//   std::cout << "Dumping the architecture " << std::endl;
//   Architecture::printCouplingMap(arch.getCouplingMap(), std::cout);
//   // Defining the settings of the mqt-mapper
//   Configuration settings{};
//   settings.heuristic = Heuristic::GateCountMaxDistance;
//   settings.layering = Layering::DisjointQubits;
//   settings.initialLayout = InitialLayout::Identity;
//   settings.preMappingOptimizations = false;
//   settings.postMappingOptimizations = false;
//   settings.lookaheadHeuristic = LookaheadHeuristic::None;
//   settings.debug = false;
//   settings.addMeasurementsToMappedCircuit = true;
//   // Adding the QuakeQMap pass to the PassManager
//   pm.nest<mlir::func::FuncOp>().addPass(mqss::opt::createQuakeQMapPass(arch,settings));
//   // pass to canonical form and remove non-used operations
//   pm.addPass(mlir::createCanonicalizerPass());
//   pm.addPass(mlir::createCSEPass());
//   // running the pass
//   if(mlir::failed(pm.run(mlirModule)))
//     std::runtime_error("The pass failed...");
//   #ifdef DEBUG
//     std::cout << "Mapped Circuit:\n";
//     mlirModule->dump();
//   #endif
//   // Convert the module to a string
//   std::string moduleOutput;
//   llvm::raw_string_ostream stringStream(moduleOutput);
//   mlirModule->print(stringStream);
//   // dump output to qasm
//   std::string qasmOutput = lowerQuakeCodeToOpenQASM(moduleOutput);
//   #ifdef DEBUG
//     std::cout << "QASM output module " << std::endl << qasmOutput <<
//     std::endl;
//   #endif
//   qasmStream = std::stringstream(qasmOutput);
//   qc2.import(qasmStream, qc::Format::OpenQASM2);
//
//   config.functionality.traceThreshold = 1e-2;
//   config.execution.runConstructionChecker = true;
//   config.execution.runAlternatingChecker = false;
//   config.execution.runZXChecker = false;
//   config.execution.runSimulationChecker = true;
//
//   ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
//   ecm.run();
//   std::cout << ecm.getResults() << "\n";
//   EXPECT_EQ(ecm.equivalence(),
//             ec::EquivalenceCriterion::Equivalent);
// }
//
// TEST_F(EqualityTest, TestQuakeQMapPass02){
//   // load mlir module and the golden output
//   std::string quakeModule =  getQuake("./quake/QuakeQMapPass-02.qke");
//   // get the QASM of the input module
//   std::string qasmInput = lowerQuakeCodeToOpenQASM(quakeModule);
//   #ifdef DEBUG
//     std::cout << "Input Quake Module " << std::endl << quakeModule <<
//     std::endl; std::cout << "QASM input module:"<< std::endl << qasmInput <<
//     std::endl;
//   #endif
//   // loading qc with QASM
//   std::stringstream qasmStream = std::stringstream(qasmInput);
//   qc1.import(qasmStream, qc::Format::OpenQASM2);
//   auto [mlirModule, contextPtr] = extractMLIRContext(quakeModule);
//   mlir::MLIRContext &context = *contextPtr;
//   // creating pass manager
//   mlir::PassManager pm(&context);
//   // Defining test architecture
//   Architecture arch{};
//   /*
//       3
//       / \
//    4   2
//     |   |
//     0---1
//   */
//   const CouplingMap cm = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
//                         {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
//   arch.loadCouplingMap(5, cm);
//   std::cout << "Dumping the architecture " << std::endl;
//   Architecture::printCouplingMap(arch.getCouplingMap(), std::cout);
//   // Defining the settings of the mqt-mapper
//   Configuration settings{};
//   settings.heuristic = Heuristic::GateCountMaxDistance;
//   settings.layering = Layering::DisjointQubits;
//   settings.initialLayout = InitialLayout::Identity;
//   settings.preMappingOptimizations = false;
//   settings.postMappingOptimizations = false;
//   settings.lookaheadHeuristic = LookaheadHeuristic::None;
//   settings.debug = false;
//   settings.addMeasurementsToMappedCircuit = true;
//   // Adding the QuakeQMap pass to the PassManager
//   pm.nest<mlir::func::FuncOp>().addPass(mqss::opt::createQuakeQMapPass(arch,settings));
//   // pass to canonical form and remove non-used operations
//   pm.addPass(mlir::createCanonicalizerPass());
//   pm.addPass(mlir::createCSEPass());
//   // running the pass
//   if(mlir::failed(pm.run(mlirModule)))
//     std::runtime_error("The pass failed...");
//   #ifdef DEBUG
//     std::cout << "Mapped Circuit:\n";
//     mlirModule->dump();
//   #endif
//   // Convert the module to a string
//   std::string moduleOutput;
//   llvm::raw_string_ostream stringStream(moduleOutput);
//   mlirModule->print(stringStream);
//  // dump output to qasm
//   std::string qasmOutput = lowerQuakeCodeToOpenQASM(moduleOutput);
//   #ifdef DEBUG
//     std::cout << "QASM output module " << std::endl << qasmOutput <<
//     std::endl;
//   #endif
//   qasmStream = std::stringstream(qasmOutput);
//   qc2.import(qasmStream, qc::Format::OpenQASM2);
//
//   config.functionality.traceThreshold = 1e-2;
//   config.execution.runConstructionChecker = true;
//   config.execution.runAlternatingChecker = false;
//   config.execution.runZXChecker = false;
//   config.execution.runSimulationChecker = true;
//
//   ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
//   ecm.run();
//   std::cout << ecm.getResults() << "\n";
//   EXPECT_EQ(ecm.equivalence(),
//             ec::EquivalenceCriterion::Equivalent);
// }

// Return QASM strings of the input module and the module after pass
std::tuple<std::string, std::string>
verificationTest(std::tuple<std::string, std::string,
                            std::function<std::unique_ptr<mlir::Pass>()>>
                     test) {
  std::string fileInputTest = std::get<1>(test);
  auto passMlir = std::get<2>(test);
  // Invoke the function to create the pass
  std::unique_ptr<mlir::Pass> pass = passMlir();

  // load mlir module
  std::string quakeModule = getQuake(fileInputTest);
  // get the QASM of the input module
  std::string qasmInput = lowerQuakeCodeToOpenQASM(quakeModule);

#ifdef DEBUG
  std::cout << "Input Quake Module " << std::endl << quakeModule << std::endl;
  std::cout << "QASM input module:" << std::endl << qasmInput << std::endl;
#endif
  auto [mlirModule, contextPtr] = extractMLIRContext(quakeModule);
  mlir::MLIRContext &context = *contextPtr;
  // creating pass manager
  mlir::PassManager pm(&context);
  // Adding the pass to the PassManager
  pm.addPass(std::move(pass));
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
  // dump output to qasm
  std::string qasmOutput = lowerQuakeCodeToOpenQASM(moduleOutput);
#ifdef DEBUG
  std::cout << "QASM output module " << std::endl << qasmOutput << std::endl;
#endif
  return std::make_tuple(qasmInput, qasmOutput);
}

class VerificationTestPassesMQSS
    : public ::testing::TestWithParam<
          std::tuple<std::string, std::string,
                     std::function<std::unique_ptr<mlir::Pass>()>>> {};

TEST_P(VerificationTestPassesMQSS, Run) {
  std::tuple<std::string, std::string,
             std::function<std::unique_ptr<mlir::Pass>()>>
      p = GetParam();
  std::string testName = std::get<0>(p);
  SCOPED_TRACE(testName);
  auto [qasmInput, qasmOutput] = verificationTest(p);
  // qcec objects required for verification
  qc::QuantumComputation qc1, qc2;
  ec::Configuration config{};
  std::stringstream qasmStream = std::stringstream(qasmInput);
  qc1.import(qasmStream, qc::Format::OpenQASM2);
  qasmStream = std::stringstream(qasmOutput);
  qc2.import(qasmStream, qc::Format::OpenQASM2);
  // set the configuration
  config.functionality.traceThreshold = 1e-2;
  config.execution.runConstructionChecker = true;
  config.execution.runAlternatingChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runSimulationChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

INSTANTIATE_TEST_SUITE_P(
    MQSSPassTests, VerificationTestPassesMQSS,
    ::testing::Values(
        std::make_tuple("TestCxToHCzHDecompositionPass",
                        "./quake/CxToHCzHDecompositionPass.qke",
                        []() {
                          return mqss::opt::createCxToHCzHDecompositionPass();
                        }),
        std::make_tuple("TestCzToHCxHDecompositionPass",
                        "./quake/CzToHCxHDecompositionPass.qke",
                        []() {
                          return mqss::opt::createCzToHCxHDecompositionPass();
                        }),
        std::make_tuple("TestCommuteCnotRxPass",
                        "./quake/CommuteCNotRxPass.qke",
                        []() { return mqss::opt::createCommuteCxRxPass(); }),
        std::make_tuple("TestCommuteCnotXPass", "./quake/CommuteCNotXPass.qke",
                        []() { return mqss::opt::createCommuteCxXPass(); }),
        std::make_tuple("TestCommuteCnotZPass01",
                        "./quake/CommuteCNotZPass-01.qke",
                        []() { return mqss::opt::createCommuteCxZPass(); }),
        // std::make_tuple("TestCommuteCnotZPass02",
        //                 "./quake/CommuteCNotZPass-02.qke",
        //                 []() { return mqss::opt::createCommuteCNotZPass();}),
        std::make_tuple("TestCommuteCnotZPass", "./quake/CommuteCNotZPass.qke",
                        []() { return mqss::opt::createCommuteCxZPass(); }),
        std::make_tuple("TestCommuteRxCnotPass",
                        "./quake/CommuteRxCNotPass.qke",
                        []() { return mqss::opt::createCommuteRxCxPass(); }),
        std::make_tuple("TestCommuteXCNotPass", "./quake/CommuteXCNotPass.qke",
                        []() { return mqss::opt::createCommuteXCxPass(); }),
        std::make_tuple("TestCommuteZCnotPass", "./quake/CommuteZCNotPass.qke",
                        []() { return mqss::opt::createCommuteZCxPass(); }),
        std::make_tuple("TestCommuteZCnotPass01",
                        "./quake/CommuteZCNotPass-01.qke",
                        []() { return mqss::opt::createCommuteZCxPass(); }),
        std::make_tuple("DoubleCnotCancellationPass",
                        "./quake/DoubleCnotCancellationPass.qke",
                        []() {
                          return mqss::opt::createCancellationDoubleCxPass();
                        }),
        std::make_tuple("ReverseCNotPass", "./quake/ReverseCNotPass.qke",
                        []() { return mqss::opt::createReverseCxPass(); }),
        //    std::make_tuple("NormalizeArgAnglePass",
        //                    "./quake/NormalizeArgAnglePass.qke",
        //                    []() { return
        //                    mqss::opt::createNormalizeArgAnglePass();}),
        std::make_tuple("HXHToZPass", "./quake/HXHToZPass.qke",
                        []() { return mqss::opt::createHXHToZPass(); }),
        std::make_tuple("XGateAndHadamardSwitchPass",
                        "./quake/XGateAndHadamardSwitchPass.qke",
                        []() { return mqss::opt::createSwitchXHPass(); }),
        //    std::make_tuple("YGateAndHadamardSwitchPass",
        //                    "./quake/YGateAndHadamardSwitchPass.qke",
        //                    []() { return
        //                    mqss::opt::createYGateAndHadamardSwitchPass();}),
        std::make_tuple("ZGateAndHadamardSwitchPass",
                        "./quake/ZGateAndHadamardSwitchPass.qke",
                        []() { return mqss::opt::createSwitchZHPass(); }),
        std::make_tuple("PauliGateAndHadamardSwitchPassX",
                        "./quake/XGateAndHadamardSwitchPass.qke",
                        []() { return mqss::opt::createSwitchPauliHPass(); }),
        //    std::make_tuple("PauliGateAndHadamardSwitchPassY",
        //                    "./quake/YGateAndHadamardSwitchPass.qke",
        //                    []() { return
        //                    mqss::opt::createPauliGateAndHadamardSwitchPass();}),
        std::make_tuple("PauliGateAndHadamardSwitchPassZ",
                        "./quake/ZGateAndHadamardSwitchPass.qke",
                        []() { return mqss::opt::createSwitchPauliHPass(); }),
        std::make_tuple("HZHToXPass", "./quake/HZHToXPass.qke",
                        []() { return mqss::opt::createHZHToXPass(); }),
        std::make_tuple("HadamardAndXGateSwitchPass",
                        "./quake/HadamardAndXGateSwitchPass.qke",
                        []() { return mqss::opt::createSwitchHXPass(); }),
        //    std::make_tuple("HadamardAndYGateSwitchPass",
        //                    "./quake/HadamardAndYGateSwitchPass.qke",
        //                    []() { return
        //                    mqss::opt::createHadamardAndYGateSwitchPass();}),
        std::make_tuple("HadamardAndZGateSwitchPass",
                        "./quake/HadamardAndZGateSwitchPass.qke",
                        []() { return mqss::opt::createSwitchHZPass(); }),
        //    std::make_tuple("NullRotationCancellationPass",
        //                    "./quake/NullRotationCancellationPass.qke",
        //                    []() { return
        //                    mqss::opt::createNullRotationCancellationPass();})
        std::make_tuple("SAdjZToSPass", "./quake/SAdjToSPass.qke",
                        []() { return mqss::opt::createSAdjZToSPass(); }),
        std::make_tuple("SZToSAdjPass", "./quake/SToSAdjPass.qke",
                        []() { return mqss::opt::createSZToSAdjPass(); })
        //    std::make_tuple("NormalizeArgAnglePass",
        //                    "./quake/NormalizeArgAnglePass.qke",
        //                    []() { return
        //                    mqss::opt::createNormalizeArgAnglePass();})
        ),
    [](const ::testing::TestParamInfo<VerificationTestPassesMQSS::ParamType>
           &info) {
      // Use the first element of the tuple (testName) as the custom test name
      return std::get<0>(info.param);
    });

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
