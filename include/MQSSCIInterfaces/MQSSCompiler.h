
/*
 * Copyright (c) 2026 MQSS Maintainers
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// MQSS compiler to provide a consistent interface for the
// QRM workflow. This is responsible for invoking the MQSS
// compiler with the appropriate arguments and handling the output. This will be
// deprecated once the MQSS compiler provides a consistent interface for the QRM
// workflow example.

#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mqss {

struct CompilerOptions {
  int optimization_level;
  std::string result_type;
};

class MQSSCompiler {
public:
  MQSSCompiler() = default;

  // Invokes the MQSS compiler with the provided arguments.
  // @param input_circuit The input circuit to compile.
  // @param backend_name The name of the target backend.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param qubit_connectivity The qubit connectivity map for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::string &input_circuit_path,
          const std::string &backend_name,
          const std::vector<std::string> &native_gates,
          const std::unordered_map<int, int> &qubit_connectivity,
          const CompilerOptions &opts);

  // Overloaded compile method for cases where only the backend name is
  // provided.
  // @param input_circuit The input circuit to compile.
  // @param backend_name The name of the target backend.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string> compile(const std::string &input_circuit_path,
                                     const std::string &backend_name,
                                     const CompilerOptions &opts) {
    return compile(input_circuit_path, backend_name, {}, {}, opts);
  }

  // Overloaded compile method for cases where only the input circuit and native
  // gates are provided.
  // @param input_circuit The input circuit to compile.
  // @param native_gates The list of native gates for the target quantum
  // hardware.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string>
  compile(const std::string &input_circuit,
          const std::vector<std::string> &native_gates,
          const CompilerOptions &opts) {
    return compile(input_circuit, "", native_gates, {}, opts);
  }

  // Overloaded compile method for cases where only the input circuit is
  // provided.
  // @param input_circuit The input circuit to compile.
  // @param opts The compiler options.
  // @return The compiled circuit as a string.
  std::optional<std::string> compile(const std::string &input_circuit,
                                     const CompilerOptions &opts) {
    return compile(input_circuit, "", {}, {}, opts);
  }

private:
  static std::string stripSpuriousGateDefs(const std::string &qasm);
};

} // namespace mqss
