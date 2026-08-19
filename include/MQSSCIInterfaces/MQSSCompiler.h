
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

#pragma once

#include <array>
#include <filesystem>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/OwningOpRef.h>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mqss {

// Enum of supported input formats.
enum InputType { QUAKE, CATALYSTQUANTUM };

struct InputTypeInfo {
  InputType type;
  std::string_view name;
};

// Pairs each InputType with its display name; iterated by
// getSupportedInputFormats() so the two can't drift apart.
inline constexpr std::array<InputTypeInfo, 2> kInputTypes = {{
    {QUAKE, "cudaq-quake"},
    {CATALYSTQUANTUM, "catalyst-quantum"},
}};

enum ResultType { OPENQASM2, QIR, QIRBASE, QIRADAPTIVE, QIRFULL };

enum OptLevel { O1, O2, O3 };

inline std::string resulttype_tostring(ResultType result_type) {

  switch (result_type) {
  case OPENQASM2:
    return "OpenQASM2";
  case QIR:
    return "qir";
  case QIRBASE:
    return "qir-base";
  case QIRADAPTIVE:
    return "qir-adaptive";
  case QIRFULL:
    return "qir-full";
  default:
    return "";
  }
}

struct CompilerOptions {
  OptLevel optimization_level;
  ResultType result_type;
};

class MQSSCompiler {
public:
  MQSSCompiler() = default;

  // Invokes the MQSS compiler on an input circuit read from a file.
  // @param input_circuit_path Path to the input circuit to compile.
  // @param backend_name The name of the target backend.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param qubit_connectivity The qubit connectivity map for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::filesystem::path &input_circuit_path,
          const std::string &backend_name,
          const std::vector<std::string> &native_gates,
          const std::unordered_map<int, int> &qubit_connectivity,
          const CompilerOptions &opts);

  // Overloaded compile method for cases where only the backend name is
  // provided.
  // @param input_circuit_path Path to the input circuit to compile.
  // @param backend_name The name of the target backend.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::filesystem::path &input_circuit_path,
          const std::string &backend_name, const CompilerOptions &opts) {
    return compile(input_circuit_path, backend_name, {}, {}, opts);
  }

  // Overloaded compile method for cases where only the input circuit and native
  // gates are provided.
  // @param input_circuit_path Path to the input circuit to compile.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::filesystem::path &input_circuit_path,
          const std::vector<std::string> &native_gates,
          const CompilerOptions &opts) {
    return compile(input_circuit_path, "", native_gates, {}, opts);
  }

  // Overloaded compile method for cases where only the input circuit is
  // provided.
  // @param input_circuit_path Path to the input circuit to compile.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::filesystem::path &input_circuit_path,
          const CompilerOptions &opts) {
    return compile(input_circuit_path, "", {}, {}, opts);
  }

  // Invokes the MQSS compiler on an input circuit given as raw MLIR source
  // text (e.g. already in memory, rather than on disk).
  // @param input_circuit Raw MLIR source text to compile.
  // @param backend_name The name of the target backend.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param qubit_connectivity The qubit connectivity map for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compileSource(const std::string &input_circuit,
                const std::string &backend_name,
                const std::vector<std::string> &native_gates,
                const std::unordered_map<int, int> &qubit_connectivity,
                const CompilerOptions &opts);

  // Overloaded compileSource method for cases where only the backend name is
  // provided.
  // @param input_circuit Raw MLIR source text to compile.
  // @param backend_name The name of the target backend.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string> compileSource(const std::string &input_circuit,
                                           const std::string &backend_name,
                                           const CompilerOptions &opts) {
    return compileSource(input_circuit, backend_name, {}, {}, opts);
  }

  // Overloaded compileSource method for cases where only the input circuit
  // and native gates are provided.
  // @param input_circuit Raw MLIR source text to compile.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compileSource(const std::string &input_circuit,
                const std::vector<std::string> &native_gates,
                const CompilerOptions &opts) {
    return compileSource(input_circuit, "", native_gates, {}, opts);
  }

  // Overloaded compileSource method for cases where only the input circuit
  // is provided.
  // @param input_circuit Raw MLIR source text to compile.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string> compileSource(const std::string &input_circuit,
                                           const CompilerOptions &opts) {
    return compileSource(input_circuit, "", {}, {}, opts);
  }

  static void getSupportedInputFormats() {
    for (const auto &input_type : kInputTypes)
      std::cout << input_type.name << "\n";
  }

private:
  static std::string stripSpuriousGateDefs(const std::string &qasm);

  // Shared pipeline core: runs optimization, qubit mapping, basis
  // conversion, and lowering on an already-parsed module. compile() and
  // compileSource() differ only in how they obtain `module`; both funnel
  // into this so the pipeline itself isn't duplicated between them.
  std::optional<std::string>
  compileImpl(mlir::OwningOpRef<mlir::ModuleOp> module,
              mlir::MLIRContext &context, const std::string &backend_name,
              const std::vector<std::string> &native_gates,
              const std::unordered_map<int, int> &qubit_connectivity,
              const CompilerOptions &opts);

  static mlir::OwningOpRef<mlir::ModuleOp>
  parseModuleFromFile(const std::filesystem::path &path,
                      mlir::MLIRContext &context);

  static mlir::OwningOpRef<mlir::ModuleOp>
  parseModuleFromSource(llvm::StringRef source, mlir::MLIRContext &context);
};

} // namespace mqss
