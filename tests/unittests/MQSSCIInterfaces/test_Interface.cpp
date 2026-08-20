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

#include "MQSSCIInterfaces/MQSSCompiler.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <string>

namespace {

// Path to tests/unittests/input, supplied by CMake
std::filesystem::path kBellStateCircuit =
    MQSSCI_TEST_FIXTURE_DIR "/two_qubit_bell.qke";

TEST(MQSSCIInterfacesTest, CompilesToOpenQASM2ForPlanqcBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  std::optional<std::string> qasm =
      compiler.compile(kBellStateCircuit, "planqc", opts);

  ASSERT_TRUE(qasm.has_value())
      << "compile() returned nullopt; expected a valid OpenQASM2 program";
  EXPECT_FALSE(qasm->empty());

  // Header emitted by cudaq::translateToOpenQASM for every OpenQASM2 program.
  EXPECT_NE(qasm->find("OPENQASM 2.0;"), std::string::npos);

  // The circuit allocates a qubit register and measures it.
  EXPECT_NE(qasm->find("qreg"), std::string::npos);
  EXPECT_NE(qasm->find("measure"), std::string::npos);

  // stripSpuriousGateDefs should have removed any leftover custom "gate"
  // definition blocks, leaving only the entry point's circuit body.
  EXPECT_EQ(qasm->find("gate "), std::string::npos);

  // "planqc" maps to the {rx, cz, rz} native-gate set (see library.md).
  // BasisConversionPass should have decomposed the whole circuit into
  // exactly that basis.
  EXPECT_NE(qasm->find("rz("), std::string::npos)
      << "expected at least one native rz rotation";
  EXPECT_NE(qasm->find("rx("), std::string::npos)
      << "expected at least one native rx rotation";
  EXPECT_NE(qasm->find("cz "), std::string::npos)
      << "expected the native two-qubit cz gate";

  // The fixture's single quake.x is a controlled-X (CNOT); it should have
  // been decomposed into cz, not left as a raw cx. Its quake.h should
  // likewise have been decomposed into rz/rx rotations, not left as h.
  // Matched with a leading newline so these don't false-positive on
  // substrings inside e.g. "qelib1.inc" or unrelated declarations.
  EXPECT_EQ(qasm->find("\nh "), std::string::npos)
      << "found a non-native 'h' gate; planqc's native set is {rx, cz, rz}";
  EXPECT_EQ(qasm->find("\ncx "), std::string::npos)
      << "found a non-native 'cx' gate; planqc's native set is {rx, cz, rz}";

  // Exactly one two-qubit interaction should remain, matching the fixture's
  // single quake.x.
  int cz_count = 0;
  for (size_t pos = qasm->find("cz "); pos != std::string::npos;
       pos = qasm->find("cz ", pos + 1)) {
    ++cz_count;
  }
  EXPECT_EQ(cz_count, 1);
}

TEST(MQSSCIInterfacesTest, CompileSourceMatchesCompileForSameCircuit) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  std::ifstream fixture(kBellStateCircuit);
  ASSERT_TRUE(fixture.is_open());
  std::stringstream buffer;
  buffer << fixture.rdbuf();
  std::string source = buffer.str();

  std::optional<std::string> qasm_from_path =
      compiler.compile(kBellStateCircuit, "planqc", opts);
  std::optional<std::string> qasm_from_source =
      compiler.compileSource(source, "planqc", opts);

  ASSERT_TRUE(qasm_from_path.has_value());
  ASSERT_TRUE(qasm_from_source.has_value())
      << "compileSource() returned nullopt; expected a valid OpenQASM2 "
         "program parsed from raw MLIR text";

  // compile() (path) and compileSource() (raw text) parse the exact same
  // circuit, so they should produce identical output.
  EXPECT_EQ(*qasm_from_path, *qasm_from_source);
}

TEST(MQSSCIInterfacesTest, CompileReturnsNulloptForUnsupportedBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  std::optional<std::string> qasm =
      compiler.compile(kBellStateCircuit, "not-a-real-backend", opts);

  EXPECT_FALSE(qasm.has_value())
      << "compile() should reject a backend name that isn't one of "
         "iqm, planqc, or wmi";
}

TEST(MQSSCIInterfacesTest, CompilesToQIRBaseForWmiBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::QIRBASE;

  std::optional<std::string> qir =
      compiler.compile(kBellStateCircuit, "wmi", opts);

  ASSERT_TRUE(qir.has_value())
      << "compile() returned nullopt; expected a valid QIR base-profile "
         "program";
  EXPECT_FALSE(qir->empty());

  // Lowering to QIR (LLVM IR) should have produced an actual function body,
  // and tagged the module as targeting the QIR base profile (see
  // tests/dialects/quake/WMIDecompositionToQIR.qke, which exercises the
  // same wmi + qir-base:2.0 combination at the pass level).
  EXPECT_NE(qir->find("define void @"), std::string::npos);
  EXPECT_NE(qir->find("\"qir_profiles\"=\"base_profile\""), std::string::npos);

  // "wmi" maps to the {cz, x, y, rz} native-gate set (see library.md).
  // BasisConversionPass should have decomposed the whole circuit into
  // exactly that basis before QIR lowering.
  EXPECT_NE(qir->find("call void @__quantum__qis__rz__body"), std::string::npos)
      << "expected at least one native rz rotation";
  EXPECT_NE(qir->find("call void @__quantum__qis__x__body"), std::string::npos)
      << "expected at least one native x gate";
  EXPECT_NE(qir->find("call void @__quantum__qis__cz__body"), std::string::npos)
      << "expected the native two-qubit cz gate";

  // The fixture's quake.h is not in wmi's native set and should have been
  // decomposed into rz/x rotations, not left as a QIR __quantum__qis__h call.
  EXPECT_EQ(qir->find("__quantum__qis__h__body"), std::string::npos)
      << "found a non-native 'h' gate; wmi's native set is {cz, x, y, rz}";

  // Exactly one two-qubit interaction should remain, matching the fixture's
  // single quake.x (a CNOT, decomposed into cz). Matched on "call void @..."
  // rather than the bare gate name so this doesn't also count the single
  // "declare void @__quantum__qis__cz__body" line QIR emits for the gate.
  int cz_count = 0;
  for (size_t pos = qir->find("call void @__quantum__qis__cz__body");
       pos != std::string::npos;
       pos = qir->find("call void @__quantum__qis__cz__body", pos + 1)) {
    ++cz_count;
  }
  EXPECT_EQ(cz_count, 1);
}

TEST(MQSSCIInterfacesTest, CompileSourceMatchesCompileForQIRBase) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::QIRBASE;

  std::ifstream fixture(kBellStateCircuit);
  ASSERT_TRUE(fixture.is_open());
  std::stringstream buffer;
  buffer << fixture.rdbuf();
  std::string source = buffer.str();

  std::optional<std::string> qir_from_path =
      compiler.compile(kBellStateCircuit, "wmi", opts);
  std::optional<std::string> qir_from_source =
      compiler.compileSource(source, "wmi", opts);

  ASSERT_TRUE(qir_from_path.has_value());
  ASSERT_TRUE(qir_from_source.has_value())
      << "compileSource() returned nullopt; expected a valid QIR base-profile "
         "program parsed from raw MLIR text";

  // compile() (path) and compileSource() (raw text) parse the exact same
  // circuit, so they should produce identical output — exercised here via
  // the QIR lowering branch specifically, since it's a different codepath
  // through compileImpl() than the OpenQASM2 case above.
  EXPECT_EQ(*qir_from_path, *qir_from_source);
}

} // namespace
