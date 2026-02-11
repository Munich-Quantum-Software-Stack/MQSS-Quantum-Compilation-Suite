/*******************************************************************************
 * Copyright (c) 2022 - 2026 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <mlir/Parser/Parser.h>
#include <mlir/Support/LogicalResult.h>
#include <sstream>
#include <string>

#include "Examples.hpp"
#include "Transforms.hpp"
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

#include "common/RuntimeMLIR.h"
#include "Passes/CodeGen.hpp"

using namespace llvm;
#define CUDAQ_GEN_PREFIX_NAME "__nvqpp__mlirgen__"

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

std::tuple<mlir::ModuleOp, mlir::MLIRContext *>
extractMLIRContext(const std::string &quakeModule) {
  auto contextPtr = cudaq::getOwningMLIRContext();
  mlir::MLIRContext &context = *contextPtr.get();

  // Get the quake representation of the kernel
  auto quakeCode = quakeModule;
  auto m_module = mlir::parseSourceString<mlir::ModuleOp>(quakeCode, &context);
  if (!m_module)
   std::runtime_error("Module cannot be parsed");

  return std::make_tuple(m_module.release(), contextPtr.release());
}


void mytest(std::string inputfile){
    //std::string inputfile = "./quake/PrintQuakeGatesPass.qke";
    //std::string inputfile = argv[1];
    std::string quakemodule = readFileToString(inputfile);

    auto [mlirModule, contextPtr] = extractMLIRContext(quakemodule);
    mlir::MLIRContext &context = *contextPtr;
    // creating pass manager
    std::string moduleOutput;
    mlir::PassManager pm(&context);
    llvm::raw_string_ostream stringStream(moduleOutput);
    pm.addPass(mqss::opt::createPrintQuakeGatesPass(stringStream));
    pm.run(mlirModule);
    // flush the buffer to the console
    stringStream.flush(); // ensures moduleOutput contains everything
    if(mlir::failed(pm.run(mlirModule))){
        std::runtime_error("The pass failed...");
    }
    std::cout << "My test Transformation passed!\n";
    std::cout << moduleOutput;
}

int main(int argc, char **argv){
    mytest(argv[1]);
    return 0;
}