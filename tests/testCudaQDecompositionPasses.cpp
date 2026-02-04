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
  std::vector<std::string> decompositionPatterns = std::get<3>(test);
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
  // pm.addPass(cudaq::opt::createBasisConversionPass());
  cudaq::opt::DecompositionPassOptions options;
  // options.disabledPatterns = {"s"};
  options.enabledPatterns = decompositionPatterns;
  pm.addPass(cudaq::opt::createDecompositionPass(options));
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

class BehaviouralCudaqDecompositionPasses
    : public ::testing::TestWithParam<std::tuple<
          std::string,             // name test
          std::string,             // input of the test
          std::string,             // expected output
          std::vector<std::string> // vector of Decomposition patterns
          >> {};

TEST_P(BehaviouralCudaqDecompositionPasses, Run) {
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
    DecompositionPassesTests, BehaviouralCudaqDecompositionPasses,
    ::testing::Values(
        // quake.h target
        // ───────────────────────────────────
        // quake.phased_rx(π/2, π/2) target
        // quake.phased_rx(π, 0) target
        std::make_tuple("TestHToPhasedRx",
                        "./quake/cudaq-decompositions/HToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/HToPhasedRx.qke",
                        std::vector<std::string>{"HToPhasedRx"}),
        // quake.exp_pauli(theta) target pauliWord
        // ───────────────────────────────────
        // Basis change operations, cnots, rz(theta), adjoint basis change
        // TODO: ask how this should work
        std::make_tuple(
            "TestExpPauliDecomposition",
            "./quake/cudaq-decompositions/ExpPauliDecomposition.qke",
            "./golden-cases/cudaq-decompositions/ExpPauliDecomposition.qke",
            std::vector<std::string>{"ExpPauliDecomposition"}),
        // Naive mapping of R1 to Rz, ignoring the global phase.
        // This is only expected to work with full inlining and
        // quake apply specialization.
        // TODO: apparently does nothing
        std::make_tuple("TestR1ToRz", "./quake/cudaq-decompositions/R1ToRz.qke",
                        "./golden-cases/cudaq-decompositions/R1ToRz.qke",
                        std::vector<std::string>{"R1ToRz"}),
        // quake.swap a, b
        // ───────────────────────────────────
        // quake.cnot b, a;
        // quake.cnot a, b;
        // quake.cnot b, a;
        std::make_tuple("TestSwapToCX",
                        "./quake/cudaq-decompositions/SwapToCX.qke",
                        "./golden-cases/cudaq-decompositions/SwapToCX.qke",
                        std::vector<std::string>{"SwapToCX"}),
        // quake.h control, target
        // ───────────────────────────────────
        // quake.s target;
        // quake.h target;
        // quake.t target;
        // quake.x control, target;
        // quake.t<adj> target;
        // quake.h target;
        // quake.s<adj> target;
        std::make_tuple("TestCHToCX", "./quake/cudaq-decompositions/CHToCX.qke",
                        "./golden-cases/cudaq-decompositions/CHToCX.qke",
                        std::vector<std::string>{"CHToCX"}),
        //===----------------------------------------------------------------------===//
        // SOp decompositions
        //===----------------------------------------------------------------------===//
        // quake.s target
        // ──────────────────────────────
        // phased_rx(π/2, 0) target
        // phased_rx(-π/2, π/2) target
        // phased_rx(-π/2, 0) target
        std::make_tuple("TestSToPhasedRx",
                        "./quake/cudaq-decompositions/SToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/SToPhasedRx.qke",
                        std::vector<std::string>{"SToPhasedRx"}),
        // quake.s [control] target
        // ────────────────────────────────────
        // quake.r1(π/2) [control] target
        //
        // Adding this gate equivalence will enable further decomposition via
        // other patterns such as controlled-r1 to cnot.
        std::make_tuple("TestSToR1", "./quake/cudaq-decompositions/SToR1.qke",
                        "./golden-cases/cudaq-decompositions/SToR1.qke",
                        std::vector<std::string>{"SToR1"}),
        //===----------------------------------------------------------------------===//
        // TOp decompositions
        //===----------------------------------------------------------------------===//
        // quake.t target
        // ────────────────────────────────────
        // quake.phased_rx(π/2, 0) target
        // quake.phased_rx(-π/4, π/2) target
        // quake.phased_rx(-π/2, 0) target
        std::make_tuple("TestTToPhasedRx",
                        "./quake/cudaq-decompositions/TToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/TToPhasedRx.qke",
                        std::vector<std::string>{"TToPhasedRx"}),
        // quake.t [control] target
        // ────────────────────────────────────
        // quake.r1(π/4) [control] target
        //
        // Adding this gate equivalence will enable further decomposition via
        // other patterns such as controlled-r1 to cnot.
        std::make_tuple("TestTToR1", "./quake/cudaq-decompositions/TToR1.qke",
                        "./golden-cases/cudaq-decompositions/TToR1.qke",
                        std::vector<std::string>{"TToR1"}),
        //===----------------------------------------------------------------------===//
        // XOp decompositions
        //===----------------------------------------------------------------------===//
        // quake.x [control] target
        // ──────────────────────────────────
        // quake.h target
        // quake.z [control] target
        // quake.h target
        std::make_tuple("TestCXToCZ", "./quake/cudaq-decompositions/CXToCZ.qke",
                        "./golden-cases/cudaq-decompositions/CXToCZ.qke",
                        std::vector<std::string>{"CXToCZ"}),
        // quake.x [controls] target
        // ──────────────────────────────────
        // quake.h target
        // quake.z [controls] target
        // quake.h target
        std::make_tuple("TestCCXToCCZ",
                        "./quake/cudaq-decompositions/CCXToCCZ.qke",
                        "./golden-cases/cudaq-decompositions/CCXToCCZ.qke",
                        std::vector<std::string>{"CCXToCCZ"}),
        // quake.x target
        // ───────────────────────────────
        // quake.phased_rx(π, 0) target
        std::make_tuple("TestXToPhasedRx",
                        "./quake/cudaq-decompositions/XToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/XToPhasedRx.qke",
                        std::vector<std::string>{"XToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // YOp decompositions
        //===----------------------------------------------------------------------===//

        // quake.y target
        // ─────────────────────────────────
        // quake.phased_rx(π, -π/2) target
        std::make_tuple("TestYToPhasedRx",
                        "./quake/cudaq-decompositions/YToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/YToPhasedRx.qke",
                        std::vector<std::string>{"YToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // ZOp decompositions
        //===----------------------------------------------------------------------===//

        //                                                                  ┌───┐
        //  ───●────  ──────────────●───────────────────●──────●─────────●──┤ T
        //  ├
        //     │                    │                   │      │         │ └───┘
        //     │                    │                   │ ┌─┴─┐┌───┐┌─┴─┐┌───┐
        //  ───●─── = ────●─────────┼─────────●─────────┼────┤ X ├┤ ┴ ├┤ X ├┤ T
        //  ├
        //     │          │         │         │         │ └───┘└───┘└───┘└───┘
        //   ┌─┴─┐      ┌─┴─┐┌───┐┌─┴─┐┌───┐┌─┴─┐┌───┐┌─┴─┐ ┌───┐
        //  ─┤ z ├─   ──┤ X ├┤ ┴ ├┤ X ├┤ T ├┤ X ├┤ ┴ ├┤ X ├─────────────────┤ T
        //  ├
        //   └───┘      └───┘└───┘└───┘└───┘└───┘└───┘└───┘ └───┘
        //
        // NOTE: `┴` denotes the adjoint of `T`.
        std::make_tuple("TestCCZToCX",
                        "./quake/cudaq-decompositions/CCZToCX.qke",
                        "./golden-cases/cudaq-decompositions/CCZToCX.qke",
                        std::vector<std::string>{"CCZToCX"}),
        // quake.z [control] target
        // ──────────────────────────────────
        // quake.h target
        // quake.x [control] target
        // quake.h target
        std::make_tuple("TestCZToCX", "./quake/cudaq-decompositions/CZToCX.qke",
                        "./golden-cases/cudaq-decompositions/CZToCX.qke",
                        std::vector<std::string>{"CZToCX"}),
        // quake.z target
        // ──────────────────────────────────
        // quake.phased_rx(π/2, 0) target
        // quake.phased_rx(-π, π/2) target
        // quake.phased_rx(-π/2, 0) target
        std::make_tuple("TestZToPhasedRx",
                        "./quake/cudaq-decompositions/ZToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/ZToPhasedRx.qke",
                        std::vector<std::string>{"ZToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // R1Op decompositions
        //===----------------------------------------------------------------------===//
        // quake.r1(λ) [control] target
        // ───────────────────────────────
        // quake.r1(λ/2) control
        // quake.x [control] target
        // quake.r1(-λ/2) target
        // quake.x [control] target
        // quake.r1(λ/2) target
        std::make_tuple("TestCR1ToCX",
                        "./quake/cudaq-decompositions/CR1ToCX.qke",
                        "./golden-cases/cudaq-decompositions/CR1ToCX.qke",
                        std::vector<std::string>{"CR1ToCX"}),
        // quake.r1(λ) target
        // ──────────────────────────────────
        // quake.phased_rx(π/2, 0) target
        // quake.phased_rx(-λ, π/2) target
        // quake.phased_rx(-π/2, 0) target
        std::make_tuple("TestR1ToPhasedRx",
                        "./quake/cudaq-decompositions/R1ToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/R1ToPhasedRx.qke",
                        std::vector<std::string>{"R1ToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // RxOp decompositions
        //===----------------------------------------------------------------------===//

        // quake.rx(θ) [control] target
        // ───────────────────────────────
        // quake.s target
        // quake.x [control] target
        // quake.ry(-θ/2) target
        // quake.x [control] target
        // quake.ry(θ/2) target
        // quake.rz(-π/2) target
        std::make_tuple("TestCRxToCX",
                        "./quake/cudaq-decompositions/CRxToCX.qke",
                        "./golden-cases/cudaq-decompositions/CRxToCX.qke",
                        std::vector<std::string>{"CRxToCX"}),
        // quake.rx(θ) target
        // ───────────────────────────────
        // quake.phased_rx(θ, 0) target
        std::make_tuple("TestRxToPhasedRx",
                        "./quake/cudaq-decompositions/RxToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/RxToPhasedRx.qke",
                        std::vector<std::string>{"RxToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // RyOp decompositions
        //===----------------------------------------------------------------------===//

        // quake.ry(θ) [control] target
        // ───────────────────────────────
        // quake.ry(θ/2) target
        // quake.x [control] target
        // quake.ry(-θ/2) target
        // quake.x [control] target
        std::make_tuple("TestCRyToCX",
                        "./quake/cudaq-decompositions/CRyToCX.qke",
                        "./golden-cases/cudaq-decompositions/CRyToCX.qke",
                        std::vector<std::string>{"CRyToCX"}),
        // quake.ry(θ) target
        // ─────────────────────────────────
        // quake.phased_rx(θ, π/2) target
        std::make_tuple("TestRyToPhasedRx",
                        "./quake/cudaq-decompositions/RyToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/RyToPhasedRx.qke",
                        std::vector<std::string>{"RyToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // RzOp decompositions
        //===----------------------------------------------------------------------===//

        // quake.rz(λ) [control] target
        // ───────────────────────────────
        // quake.rz(λ/2) target
        // quake.x [control] target
        // quake.rz(-λ/2) target
        // quake.x [control] target
        std::make_tuple("TestCRzToCX",
                        "./quake/cudaq-decompositions/CRzToCX.qke",
                        "./golden-cases/cudaq-decompositions/CRzToCX.qke",
                        std::vector<std::string>{"CRzToCX"}),
        // quake.rz(θ) target
        // ──────────────────────────────────
        // quake.phased_rx(π/2, 0) target
        // quake.phased_rx(-θ, π/2) target
        // quake.phased_rx(-π/2, 0) target
        std::make_tuple("TestRzToPhasedRx",
                        "./quake/cudaq-decompositions/RzToPhasedRx.qke",
                        "./golden-cases/cudaq-decompositions/RzToPhasedRx.qke",
                        std::vector<std::string>{"RzToPhasedRx"}),
        //===----------------------------------------------------------------------===//
        // U3Op decompositions
        //===----------------------------------------------------------------------===//

        // quake.u3(θ,ϕ,λ) target
        // ──────────────────────────────────
        // quake.rz(λ) target
        // quake.rx(π/2) target
        // quake.rz(θ) target
        // quake.rx(-π/2) target
        // quake.rz(ϕ) target
        std::make_tuple("TestU3ToRotations",
                        "./quake/cudaq-decompositions/U3ToRotations.qke",
                        "./golden-cases/cudaq-decompositions/U3ToRotations.qke",
                        std::vector<std::string>{"U3ToRotations"})),
    [](const ::testing::TestParamInfo<
        BehaviouralCudaqDecompositionPasses::ParamType> &info) {
      // Use the first element of the tuple (testName) as the custom test name
      return std::get<0>(info.param);
    });

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
