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
#include "Passes/Transforms/Dialects.h"
#include "Passes/Verification/Instrumentation.h"

#include <cudaq/Optimizer/Dialect/Quake/QuakeOps.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/DialectRegistry.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Parser/Parser.h>
#include <mlir/Pass/Pass.h>
#include <mlir/Pass/PassManager.h>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

namespace {

// Path to tests/unittests/input, supplied by CMake
std::filesystem::path kBellStateCircuit =
    MQSSCI_TEST_FIXTURE_DIR "/two_qubit_bell.qke";

std::filesystem::path kGHZCircuit = MQSSCI_TEST_FIXTURE_DIR "/ghz.qke";

std::filesystem::path kBellStateQASM2 =
    MQSSCI_TEST_FIXTURE_DIR "/two_qubit_bell.qasm";

std::filesystem::path kGHZQASM2 = MQSSCI_TEST_FIXTURE_DIR "/ghz.qasm";

// Measures a whole 2-qubit register in one `quake.mz` (rather than
// per-qubit, like kBellStateCircuit) -- the shape that used to make
// QuakeDialectToIQMJSONPass silently drop every qubit but the first, since
// IQM JSON has no "measure this whole register" instruction. See
// ExpandVeqMeasurement in QuakeExpandMeasurements.cpp, which now splits it
// into one `quake.mz` per qubit before JSON emission.
std::filesystem::path kMultiRotationCircuit =
    MQSSCI_TEST_FIXTURE_DIR "/multi_rotation_circuit.qke";

// Native-gate sets of the devices exercised below. MQSSCompiler no longer
// maps backend names to gate sets, so callers pass these explicitly.
const std::vector<std::string> kIqmNativeGates{"prx", "cz"};
const std::vector<std::string> kPlanqcNativeGates{"rx", "cz", "rz"};
const std::vector<std::string> kWmiNativeGates{"cz", "x", "y", "rz", "sx"};

// Returns the contents of a fixture file, or an empty string if it can't be
// opened.
std::string readFixture(const std::filesystem::path &path) {
  std::ifstream fixture(path);
  std::stringstream buffer;
  buffer << fixture.rdbuf();
  return buffer.str();
}

TEST(MQSSCIInterfacesTest, CompilesToOpenQASM2ForPlanqcBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> qasm =
      compiler.compileSource(source, kPlanqcNativeGates, opts);

  ASSERT_TRUE(qasm.has_value())
      << "compileSource() returned nullopt; expected a valid OpenQASM2 "
         "program";
  EXPECT_FALSE(qasm->empty());

  // Header emitted by cudaq::translateToOpenQASM for every OpenQASM2 program.
  EXPECT_NE(qasm->find("OPENQASM 2.0;"), std::string::npos);

  // The circuit allocates a qubit register and measures it.
  EXPECT_NE(qasm->find("qreg"), std::string::npos);
  EXPECT_NE(qasm->find("measure"), std::string::npos);

  // stripSpuriousGateDefs should have removed any leftover custom "gate"
  // definition blocks, leaving only the entry point's circuit body.
  EXPECT_EQ(qasm->find("gate "), std::string::npos);

  // planqc's native-gate set is {rx, cz, rz}. BasisConversionPass should
  // have decomposed the whole circuit into exactly that basis.
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
      compiler.compile(kBellStateCircuit, kPlanqcNativeGates, opts);
  std::optional<std::string> qasm_from_source =
      compiler.compileSource(source, kPlanqcNativeGates, opts);

  ASSERT_TRUE(qasm_from_path.has_value());
  ASSERT_TRUE(qasm_from_source.has_value())
      << "compileSource() returned nullopt; expected a valid OpenQASM2 "
         "program parsed from raw MLIR text";

  // compile() (path) and compileSource() (raw text) parse the exact same
  // circuit, so they should produce identical output.
  EXPECT_EQ(*qasm_from_path, *qasm_from_source);
}

TEST(MQSSCIInterfacesTest, RestoresCallerGateNameForLRZBackend) {
  // "r" is an alias for BasisConversionPass's own "prx" mnemonic (see
  // resolveMnemonicAlias), which cudaq's QASM2 emitter writes as
  // "phased_rx". For the LRZ backend, the output must be renamed back to
  // the caller's own "r" spelling, and "r" -- which qelib1.inc doesn't
  // define -- must be declared with a "gate r(...)" definition.
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  // "r" is listed twice to check it's still only declared once.
  const std::vector<std::string> nativeGates{"r", "cz", "r"};
  std::optional<std::string> qasm =
      compiler.compileSource(source, "LRZ", nativeGates, {}, opts);

  ASSERT_TRUE(qasm.has_value())
      << "compileSource() returned nullopt; expected a valid OpenQASM2 "
         "program";
  EXPECT_NE(qasm->find("\nr("), std::string::npos)
      << "expected the legalized H gate rendered as 'r', the caller's own "
         "name for it";
  EXPECT_EQ(qasm->find("phased_rx("), std::string::npos)
      << "found cudaq's internal 'phased_rx' spelling; the alias didn't "
         "round-trip to the output";

  // Exactly one definition of "r", placed after the qelib1.inc include and
  // before the first call to it.
  const std::string gateDef = "\ngate r(";
  const size_t defPos = qasm->find(gateDef);
  ASSERT_NE(defPos, std::string::npos)
      << "expected a 'gate r(...)' definition, since qelib1.inc doesn't "
         "define r";
  EXPECT_EQ(qasm->find(gateDef, defPos + 1), std::string::npos)
      << "'r' is defined more than once";
  EXPECT_LT(qasm->find("include \"qelib1.inc\";"), defPos)
      << "the 'r' definition must come after the qelib1.inc include";
  EXPECT_LT(defPos, qasm->find("\nr("))
      << "the 'r' definition must come before the first call to r";

  // The definition must be built on OpenQASM2's built-in U, not qelib1.inc's
  // lowercase u: the original qelib1.inc (used by strict parsers such as
  // qiskit.qasm2.loads) doesn't define u.
  const std::string defLine =
      qasm->substr(defPos + 1, qasm->find('\n', defPos + 1) - (defPos + 1));
  EXPECT_NE(defLine.find("{ U("), std::string::npos)
      << "expected the 'r' definition to use the built-in U gate, got: "
      << defLine;
  EXPECT_EQ(defLine.find("u("), std::string::npos)
      << "the 'r' definition uses lowercase u, which strict OpenQASM2 "
         "parsers reject; got: "
      << defLine;

  // cz is already defined by qelib1.inc; redefining it is invalid OpenQASM2.
  EXPECT_EQ(qasm->find("gate cz"), std::string::npos)
      << "found a 'gate cz' definition; cz is already defined in qelib1.inc";
}

TEST(MQSSCIInterfacesTest, KeepsQasmGateNameForNonLRZBackend) {
  // Same circuit and native-gate set as RestoresCallerGateNameForLRZBackend,
  // but with any backend name other than LRZ (including none, and a
  // wrongly-cased "lrz"): the compile still succeeds and "r" is still
  // resolved to "prx" for decomposition, but the output is left with cudaq's
  // own "phased_rx" spelling rather than renamed back to "r".
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  const std::vector<std::string> nativeGates{"r", "cz"};
  for (const std::string backend : {"", "iqm", "lrz", "not-a-real-backend"}) {
    std::optional<std::string> qasm =
        compiler.compileSource(source, backend, nativeGates, {}, opts);

    ASSERT_TRUE(qasm.has_value())
        << "compileSource() returned nullopt for backend '" << backend
        << "'; expected a valid OpenQASM2 program";
    EXPECT_NE(qasm->find("phased_rx("), std::string::npos)
        << "expected the legalized H gate rendered as 'phased_rx' for "
           "backend '"
        << backend << "'";
    EXPECT_EQ(qasm->find("\nr("), std::string::npos)
        << "found 'r' for backend '" << backend
        << "'; aliases should only be renamed back for LRZ";
  }
}

TEST(MQSSCIInterfacesTest, CompilesToQIRBaseForWmiBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::QIRBASE;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> qir =
      compiler.compileSource(source, kWmiNativeGates, opts);

  ASSERT_TRUE(qir.has_value())
      << "compileSource() returned nullopt; expected a valid QIR base-profile "
         "program";
  EXPECT_FALSE(qir->empty());

  // Lowering to QIR (LLVM IR) should have produced an actual function body,
  // and tagged the module as targeting the QIR base profile (see
  // tests/dialects/quake/WMIDecompositionToQIR.qke, which exercises the
  // same wmi + qir-base:2.0 combination at the pass level).
  EXPECT_NE(qir->find("define void @"), std::string::npos);
  EXPECT_NE(qir->find("\"qir_profiles\"=\"base_profile\""), std::string::npos);

  // wmi's native-gate set is {cz, x, y, rz, sx}. BasisConversionPass should
  // have decomposed the whole circuit into exactly that basis before QIR
  // lowering.
  EXPECT_NE(qir->find("call void @__quantum__qis__rz__body"), std::string::npos)
      << "expected at least one native rz rotation";
  EXPECT_NE(qir->find("call void @__quantum__qis__rx__body"), std::string::npos)
      << "expected at least one native x gate";
  EXPECT_NE(qir->find("call void @__quantum__qis__cz__body"), std::string::npos)
      << "expected the native two-qubit cz gate";

  // The fixture's quake.h is not in wmi's native set and should have been
  // decomposed into rz/x rotations, not left as a QIR __quantum__qis__h call.
  EXPECT_EQ(qir->find("__quantum__qis__h__body"), std::string::npos)
      << "found a non-native 'h' gate; wmi's native set is {cz, x, y, rz, sx}";

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
      compiler.compile(kBellStateCircuit, kWmiNativeGates, opts);
  std::optional<std::string> qir_from_source =
      compiler.compileSource(source, kWmiNativeGates, opts);

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

TEST(MQSSCIInterfacesTest, CompilesToIQMJSONForIqmBackend) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::IQMJSON;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> json =
      compiler.compileSource(source, kIqmNativeGates, opts);

  ASSERT_TRUE(json.has_value())
      << "compileSource() returned nullopt; expected a valid IQM JSON program";
  EXPECT_FALSE(json->empty());

  // The circuit's entry-point function name should be carried through as the
  // JSON program's name.
  EXPECT_NE(json->find("\"name\": \"__nvqpp__mlirgen__testILm2EE\""),
            std::string::npos);

  // IQM's native-gate set is {prx, cz}. BasisConversionPass should have
  // decomposed the whole circuit into exactly that basis, which IQM's JSON
  // emitter renders as "prx"/"cz".
  // See tests/dialects/quake/IQMTranspileToIQMJSON.qke for the same
  // decomposition pattern checked at the pass level.
  EXPECT_NE(json->find("\"name\": \"prx\""), std::string::npos)
      << "expected at least one native prx (\"prx\") gate";
  EXPECT_NE(json->find("\"name\": \"cz\""), std::string::npos)
      << "expected the native two-qubit cz gate";

  // The fixture's single quake.x is a controlled-X (CNOT) between QB1 and
  // QB2; it should have been decomposed into cz, not left as a raw cx.
  // Rather than matching the pretty-printed "qubits" array verbatim
  // (fragile against indentation changes), just check both qubit names
  // appear within the cz instruction's own JSON object.
  size_t cz_pos = json->find("\"name\": \"cz\"");
  ASSERT_NE(cz_pos, std::string::npos);
  std::string cz_instruction = json->substr(cz_pos, 150);
  EXPECT_NE(cz_instruction.find("\"QB1\""), std::string::npos)
      << "expected the native cz gate to act on QB1";
  EXPECT_NE(cz_instruction.find("\"QB2\""), std::string::npos)
      << "expected the native cz gate to act on QB2";

  // The circuit measures its qubits; at least one measurement instruction
  // should be present in the output.
  EXPECT_NE(json->find("\"name\": \"measure\""), std::string::npos)
      << "expected a measurement instruction";
  EXPECT_NE(json->find("\"key\": \"m_QB1\""), std::string::npos)
      << "expected a measurement instruction keyed to QB1";
}

TEST(MQSSCIInterfacesTest, CompilesToIQMJSONForExplicitNativeGateSet) {
  // Same circuit/assertions as CompilesToIQMJSONForIqmBackend, but goes
  // through compile() (path) rather than compileSource(), and adds a
  // native gate ("prx_12") BasisConversionPass has no case for, which must
  // be tolerated rather than failing the compile.
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::IQMJSON;

  const std::vector<std::string> nativeGates{"prx", "cz", "prx_12"};
  std::optional<std::string> json =
      compiler.compile(kBellStateCircuit, nativeGates, opts);

  ASSERT_TRUE(json.has_value())
      << "compile() returned nullopt; expected a valid IQM JSON program";
  EXPECT_FALSE(json->empty());

  // The circuit's entry-point function name should be carried through as the
  // JSON program's name.
  EXPECT_NE(json->find("\"name\": \"__nvqpp__mlirgen__testILm2EE\""),
            std::string::npos);

  // BasisConversionPass should have decomposed the whole circuit into
  // exactly the {prx, cz} set passed in, which IQM's JSON emitter renders as
  // "prx"/"cz" -- identical output to CompilesToIQMJSONForIqmBackend.
  EXPECT_NE(json->find("\"name\": \"prx\""), std::string::npos)
      << "expected at least one native prx (\"prx\") gate";
  EXPECT_NE(json->find("\"name\": \"cz\""), std::string::npos)
      << "expected the native two-qubit cz gate";

  // The fixture's single quake.x is a controlled-X (CNOT) between QB1 and
  // QB2; it should have been decomposed into cz, not left as a raw cx.
  // Rather than matching the pretty-printed "qubits" array verbatim
  // (fragile against indentation changes), just check both qubit names
  // appear within the cz instruction's own JSON object.
  size_t cz_pos = json->find("\"name\": \"cz\"");
  ASSERT_NE(cz_pos, std::string::npos);
  std::string cz_instruction = json->substr(cz_pos, 150);
  EXPECT_NE(cz_instruction.find("\"QB1\""), std::string::npos)
      << "expected the native cz gate to act on QB1";
  EXPECT_NE(cz_instruction.find("\"QB2\""), std::string::npos)
      << "expected the native cz gate to act on QB2";

  // The circuit measures its qubits; at least one measurement instruction
  // should be present in the output.
  EXPECT_NE(json->find("\"name\": \"measure\""), std::string::npos)
      << "expected a measurement instruction";
  EXPECT_NE(json->find("\"key\": \"m_QB1\""), std::string::npos)
      << "expected a measurement instruction keyed to QB1";
}

TEST(MQSSCIInterfacesTest, CompilesToIQMJSONForWholeRegisterMeasurement) {
  // Regression test: kMultiRotationCircuit measures both qubits in a single
  // `quake.mz` on the whole register, rather than one `quake.mz` per qubit
  // (see kBellStateCircuit above). Before ExpandVeqMeasurement,
  // QuakeDialectToIQMJSONPass translated that as exactly one "measure"
  // instruction keyed to the register's first qubit, silently dropping every
  // other qubit -- IQM's own circuit validator rejects the result outright
  // once a circuit has more than one such call, reporting the reused key as
  // a duplicate ("Measurement key 'm_QB1' is not unique").
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::IQMJSON;

  const std::string source = readFixture(kMultiRotationCircuit);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> json =
      compiler.compileSource(source, kIqmNativeGates, opts);

  ASSERT_TRUE(json.has_value())
      << "compileSource() returned nullopt; expected a valid IQM JSON program";
  EXPECT_FALSE(json->empty());

  EXPECT_NE(json->find("\"name\": \"__nvqpp__mlirgen__rotations\""),
            std::string::npos);

  // Exactly one measurement instruction per qubit, each with its own,
  // distinct key -- not one dropped, and not both collapsed onto the same
  // key.
  EXPECT_NE(json->find("\"key\": \"m_QB1\""), std::string::npos)
      << "expected a measurement instruction keyed to QB1";
  EXPECT_NE(json->find("\"key\": \"m_QB2\""), std::string::npos)
      << "expected a measurement instruction keyed to QB2";

  auto countOccurrences = [](const std::string &haystack,
                             const std::string &needle) {
    int count = 0;
    for (size_t pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + 1))
      ++count;
    return count;
  };
  EXPECT_EQ(countOccurrences(*json, "\"name\": \"measure\""), 2)
      << "expected exactly one measurement instruction per qubit";
  EXPECT_EQ(countOccurrences(*json, "\"key\": \"m_QB1\""), 1)
      << "m_QB1 should be used for exactly one measurement, not duplicated "
         "across both qubits";
  EXPECT_EQ(countOccurrences(*json, "\"key\": \"m_QB2\""), 1)
      << "m_QB2 should be used for exactly one measurement, not duplicated "
         "across both qubits";
}

TEST(MQSSCIInterfacesTest, DefaultQubitMappingTest) {
  // Trivial single-qubit circuit -- content is irrelevant to this test; what
  // matters is that it allocates fewer qubits than the coupling map below.
  const std::string circuit = R"(
    func.func @__nvqpp__mlirgen__k() attributes {"cudaq-entrypoint", "cudaq-kernel"} {
      %q0 = quake.alloca !quake.ref
      quake.h %q0 : (!quake.ref) -> ()
      %m = quake.mz %q0 : (!quake.ref) -> !quake.measure
      return
    }
  )";

  // A minimal 6-qubit linear chain: the circuit only uses 1 of these 6
  // qubits, so CommonMappingPass must derive the device's qubit count from
  // the coupling map itself (not from the circuit) and map onto it without
  // throwing.
  const std::vector<std::pair<std::uint32_t, std::uint32_t>> connectivity{
      {0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
      {3, 2}, {3, 4}, {4, 3}, {4, 5}, {5, 4}};
  const std::vector<std::string> nativeGates{"rx", "ry", "rz", "cx"};

  mqss::mqssci::MQSSCompiler compiler;
  const mqss::mqssci::CompilerOptions opts{mqss::mqssci::OptLevel::O1,
                                           mqss::mqssci::ResultFormat::QIRBASE};

  // If CommonMappingPass misderives the device's qubit count, this throws
  // instead of returning nullopt; gtest reports an uncaught exception as a
  // test failure, so no explicit try/catch is needed here.
  std::optional<std::string> qir =
      compiler.compileSource(circuit, "", nativeGates, connectivity, opts);

  ASSERT_TRUE(qir.has_value())
      << "compileSource() returned nullopt; expected a valid QIR base-profile "
         "program";
  EXPECT_FALSE(qir->empty());
}

TEST(MQSSCIInterfacesTest, CompileSucceedsWithVerificationEnabled) {
  // Regression test: BasisConversionPass's H decomposition and compileImpl's
  // repeated-pm.run() structure both used to break circuit equivalence (see
  // develop-guide/verification.md). With both fixed, a real, correct
  // compilation should pass verification cleanly rather than tripping the
  // signalPassFailure() path added to VerifyPassInstrumentation::runAfterPass.
  mqss::mqssci::MQSSCompiler compiler;
  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;
  opts.verify = true;

  const std::string source = readFixture(kBellStateCircuit);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> qasm =
      compiler.compileSource(source, kPlanqcNativeGates, opts);

  ASSERT_TRUE(qasm.has_value())
      << "compileSource() returned nullopt with opts.verify = true; "
         "BasisConversionPass's output should be equivalent to the input "
         "circuit, so verification should not fail the compile";
  EXPECT_FALSE(qasm->empty());
}

// A deliberately non-equivalence-preserving pass: erases the first
// controlled-X it finds. Used only to give VerifyPassInstrumentation a
// pass it's guaranteed to be able to catch, without depending on any real
// MQSS pass having a defect (which would make this test fragile -- passing
// or failing based on unrelated bugs elsewhere rather than on whether
// verification itself works).
struct EraseFirstCNOTPass
    : public mlir::PassWrapper<EraseFirstCNOTPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
  void runOnOperation() override {
    getOperation().walk([&](quake::XOp op) {
      if (!op.getControls().empty()) {
        op.erase();
        return mlir::WalkResult::interrupt();
      }
      return mlir::WalkResult::advance();
    });
  }
};

TEST(MQSSCIInterfacesTest, VerificationCatchesNonEquivalentPass) {
  // This exercises VerifyPassInstrumentation directly (the mechanism
  // MQSSCompiler wires into its BasisConversion pass manager when
  // opts.verify is set) rather than going through MQSSCompiler::compile(),
  // since there's no reliable way to force a genuine equivalence failure
  // through passes that are themselves correct.
  mlir::DialectRegistry registry;
  mqss::mqssci::opt::registerMQSSDialects(registry);
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  std::ifstream fixture(kBellStateCircuit);
  ASSERT_TRUE(fixture.is_open());
  std::stringstream buffer;
  buffer << fixture.rdbuf();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(buffer.str(), &context);
  ASSERT_TRUE(module);

  mlir::PassManager pm(&context);
  pm.addPass(std::make_unique<EraseFirstCNOTPass>());
  llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy> snapshot;

  // Pin the checker portfolio to just the alternating (DD-based, exact)
  // checker. The default portfolio also races the simulation checker, which
  // is randomly seeded and probabilistic -- for a circuit this small it can
  // occasionally report "probably equivalent" for a genuinely broken
  // transformation, which made this test flaky across separate process runs
  // (confirmed empirically: same scenario, same binary, different outcome).
  ec::Configuration config;
  config.execution.runAlternatingChecker = true;
  config.execution.runSimulationChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runConstructionChecker = false;

  pm.addInstrumentation(
      std::make_unique<mqss::mqssci::verify::VerifyPassInstrumentation>(
          std::move(snapshot), config));

  EXPECT_TRUE(mlir::failed(pm.run(*module)))
      << "EraseFirstCNOTPass removes a gate, which changes what the "
         "circuit computes; VerifyPassInstrumentation should detect the "
         "mismatch and call signalPassFailure(), making pm.run() fail";
}

TEST(MQSSCIInterfacesTest, TestsMappingAndTranspilationFor52QubitCircuit) {
  const mqss::mqssci::CompilerOptions opts{
      .optimization_level = mqss::mqssci::OptLevel::O1,
      .result_format = mqss::mqssci::OPENQASM2,
  };

  std::ifstream fixture(kGHZCircuit);
  ASSERT_TRUE(fixture.is_open());
  std::stringstream buffer;
  buffer << fixture.rdbuf();
  std::string source = buffer.str();

  mqss::mqssci::MQSSCompiler compiler;
  // This device's native gates, per IQM's own documentation: "arbitrary X
  // and Y rotations" as the single-qubit native gate -- i.e. a rotation by
  // angle theta about an axis that's an arbitrary combination of X and Y,
  // parameterized by phi. That's exactly phased_rx's definition
  // (R(theta, phi) = exp(-i*theta/2*(cos(phi)*X + sin(phi)*Y))), i.e.
  // BasisConversionPass's "prx". The device calls it "r", which the LRZ
  // backend resolves to "prx" and renames back to "r" in the output.
  // "id"/"if_else"/ "reset" are real device capabilities BasisConversionPass
  // has no case for yet (the same gap already flagged for WMI's "id"), but are
  // inert for this particular circuit since ghz.qke never produces ops that
  // would classify as any of them.
  const std::vector<std::string> native_gates = {"id",      "measure", "r",
                                                 "if_else", "reset",   "cz"};

  const std::vector<std::pair<std::uint32_t, std::uint32_t>>
      qubit_connectivity = {
          {0, 1},   {0, 4},   {3, 2},   {3, 4},   {3, 9},   {5, 1},   {5, 4},
          {5, 6},   {5, 11},  {8, 2},   {8, 7},   {8, 9},   {10, 4},  {10, 9},
          {10, 11}, {10, 17}, {12, 6},  {12, 11}, {12, 13}, {12, 19}, {15, 7},
          {15, 14}, {15, 22}, {16, 9},  {16, 17}, {16, 24}, {18, 11}, {18, 17},
          {18, 19}, {18, 26}, {20, 13}, {20, 19}, {20, 28}, {21, 14}, {21, 22},
          {23, 22}, {23, 24}, {23, 31}, {25, 17}, {25, 24}, {25, 26}, {25, 32},
          {27, 19}, {27, 26}, {27, 28}, {27, 34}, {29, 28}, {29, 36}, {30, 22},
          {30, 31}, {30, 37}, {33, 26}, {33, 32}, {33, 34}, {33, 41}, {35, 28},
          {35, 34}, {35, 36}, {35, 43}, {38, 31}, {38, 37}, {38, 39}, {38, 44},
          {40, 32}, {40, 39}, {40, 41}, {40, 46}, {42, 34}, {42, 41}, {42, 43},
          {42, 48}, {45, 39}, {45, 44}, {45, 46}, {45, 49}, {47, 41}, {47, 46},
          {47, 48}, {47, 51}, {50, 46}, {50, 49}, {50, 51},
      };

  auto compiled = compiler.compileSource(source, "LRZ", native_gates,
                                         qubit_connectivity, opts);

  ASSERT_TRUE(compiled.has_value())
      << "compileSource() returned nullopt; expected a valid OpenQASM2 "
         "program for the 3-qubit GHZ circuit mapped onto the 52-qubit "
         "device";
  EXPECT_FALSE(compiled->empty());

  EXPECT_NE(compiled->find("OPENQASM 2.0;"), std::string::npos);

  // qubit_connectivity's highest-numbered qubit is 51, so the allocated
  // register must be sized 52 -- not the logical circuit's 3 qubits, and not
  // the count of *distinct* qubits mentioned in the coupling map (which
  // would undercount whenever indices aren't contiguous, e.g. this map never
  // mentions qubit 2 on its own -- only paired with 3 and 8).
  EXPECT_NE(compiled->find("qreg var0[52];"), std::string::npos);

  // "prx" (IQM's "arbitrary X and Y rotations", i.e. phased_rx) and "cz" are
  // mnemonics BasisConversionPass does understand ("r" aliases to its own
  // "prx" mnemonic -- see resolveMnemonicAlias), and together they fully
  // legalize ghz.qke's H and both CNOTs (H -> prx; CX -> H-CZ-H -> prx/cz),
  // so no "h"/"cx"/"swap" should remain in the output -- everything must
  // have been rewritten into this native set.
  EXPECT_EQ(compiled->find("\nh "), std::string::npos)
      << "found a non-native 'h' gate; the requested native set is "
         "{id, measure, r, if_else, reset, cz}";
  EXPECT_EQ(compiled->find("\ncx "), std::string::npos)
      << "found a non-native 'cx' gate";
  EXPECT_EQ(compiled->find("\nswap "), std::string::npos)
      << "found a non-native 'swap' gate";

  // The device reports this gate as "r", not BasisConversionPass's own
  // "prx"/cudaq's "phased_rx" -- the emitted QASM2 must round-trip back to
  // the caller's own spelling, not leak the pass's internal mnemonic or
  // cudaq's op name.
  EXPECT_NE(compiled->find("\nr("), std::string::npos)
      << "expected ghz.qke's H gate, legalized and rendered back as 'r', "
         "the device's own name for this gate";
  EXPECT_EQ(compiled->find("phased_rx("), std::string::npos)
      << "found cudaq's internal 'phased_rx' spelling instead of the "
         "device's own 'r' name -- the alias didn't round-trip to the "
         "output";

  int czCount = 0;
  for (size_t pos = compiled->find("cz "); pos != std::string::npos;
       pos = compiled->find("cz ", pos + 1))
    ++czCount;
  EXPECT_GT(czCount, 0)
      << "expected at least one native cz gate from legalizing ghz.qke's "
         "CX(q0,q1), CX(q1,q2) chain";

  int measureCount = 0;
  for (size_t pos = compiled->find("measure "); pos != std::string::npos;
       pos = compiled->find("measure ", pos + 1))
    ++measureCount;
  EXPECT_EQ(measureCount, 3)
      << "expected one measurement per logical qubit in the GHZ circuit";

  // Every two-qubit operation (cz, the only native two-qubit mnemonic
  // requested) the mapper's routing produces must land on an actual edge of
  // qubit_connectivity -- this is what makes the mapping physically valid,
  // not just logically equivalent to ghz.qke. Checked generically (rather
  // than pinning the exact routing/decomposition sequence) so this doesn't
  // break if HeuristicMapper's routing choice legitimately changes between
  // mqt-core versions.
  auto isConnected = [&](uint32_t a, uint32_t b) {
    for (const auto &[x, y] : qubit_connectivity)
      if ((x == a && y == b) || (x == b && y == a))
        return true;
    return false;
  };

  std::regex twoQubitOpRe(R"(cz var0\[(\d+)\], var0\[(\d+)\];)");
  auto matchesBegin =
      std::sregex_iterator(compiled->begin(), compiled->end(), twoQubitOpRe);
  int twoQubitOpCount = 0;
  for (auto it = matchesBegin; it != std::sregex_iterator(); ++it) {
    uint32_t a = std::stoul((*it)[1].str());
    uint32_t b = std::stoul((*it)[2].str());
    EXPECT_TRUE(isConnected(a, b)) << "two-qubit op on (" << a << ", " << b
                                   << ") is not an edge in qubit_connectivity";
    ++twoQubitOpCount;
  }
  // Confirms the regex actually found every "cz " occurrence the earlier
  // substring count did, so the connectivity check above wasn't vacuous.
  EXPECT_EQ(twoQubitOpCount, czCount);
}

// Counts non-overlapping occurrences of `needle` in `haystack`.
int countOccurrences(const std::string &haystack, const std::string &needle) {
  int count = 0;
  for (size_t pos = haystack.find(needle); pos != std::string::npos;
       pos = haystack.find(needle, pos + needle.size()))
    ++count;
  return count;
}

// Compiles `qasm2` source for the planqc native-gate set to OpenQASM 2.
std::optional<std::string> compileQASM2ForPlanqc(const std::string &qasm2) {
  mqss::mqssci::MQSSCompiler compiler;

  mqss::mqssci::CompilerOptions opts;
  opts.optimization_level = mqss::mqssci::OptLevel::O1;
  opts.result_format = mqss::mqssci::ResultFormat::OPENQASM2;

  return compiler.compileSource(qasm2, kPlanqcNativeGates, opts);
}

TEST(MQSSCIInterfacesTest, CompilesBellStateQASM2Input) {
  const std::string source = readFixture(kBellStateQASM2);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> qasm = compileQASM2ForPlanqc(source);
  ASSERT_TRUE(qasm.has_value())
      << "compileSource() returned nullopt for an OpenQASM2 input";

  EXPECT_NE(qasm->find("qreg"), std::string::npos);
  EXPECT_EQ(countOccurrences(*qasm, "measure "), 2);

  // The Bell circuit's single cx must end up as the native cz, and its h as
  // rz/rx rotations: planqc's native set is {rx, cz, rz}.
  EXPECT_EQ(countOccurrences(*qasm, "cz "), 1);
  EXPECT_EQ(qasm->find("\nh "), std::string::npos);
  EXPECT_EQ(qasm->find("\ncx "), std::string::npos);
}

TEST(MQSSCIInterfacesTest, CompilesGHZQASM2Input) {
  const std::string source = readFixture(kGHZQASM2);
  ASSERT_FALSE(source.empty());

  std::optional<std::string> qasm = compileQASM2ForPlanqc(source);
  ASSERT_TRUE(qasm.has_value())
      << "compileSource() returned nullopt for an OpenQASM2 input";

  EXPECT_NE(qasm->find("qreg"), std::string::npos);
  // `measure q -> c;` is one measurement per qubit.
  EXPECT_EQ(countOccurrences(*qasm, "measure "), 3);
  // Two cx gates in the GHZ chain become two native cz gates.
  EXPECT_EQ(countOccurrences(*qasm, "cz "), 2);
  EXPECT_EQ(qasm->find("\nh "), std::string::npos);
  EXPECT_EQ(qasm->find("\ncx "), std::string::npos);
}

TEST(MQSSCIInterfacesTest, QASM2InputRejectsUnsupportedGate) {
  EXPECT_FALSE(compileQASM2ForPlanqc("OPENQASM 2.0;\n"
                                     "qreg q[1];\n"
                                     "foo q[0];\n")
                   .has_value());
}

TEST(MQSSCIInterfacesTest, QASM2InputRejectsOutOfRangeQubit) {
  EXPECT_FALSE(compileQASM2ForPlanqc("OPENQASM 2.0;\n"
                                     "qreg q[1];\n"
                                     "h q[1];\n")
                   .has_value());
}

TEST(MQSSCIInterfacesTest, QASM2InputRejectsOtherQASMVersions) {
  EXPECT_FALSE(compileQASM2ForPlanqc("OPENQASM 3.0;\n"
                                     "qubit[1] q;\n")
                   .has_value());
}

} // namespace
