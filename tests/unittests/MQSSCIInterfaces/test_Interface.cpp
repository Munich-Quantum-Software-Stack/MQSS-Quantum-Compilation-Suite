#include "MQSSCIInterfaces/MQSSCompiler.h"

#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace {

// Path to tests/unittests/input, supplied by CMake
auto *kBellStateCircuit = MQSSCI_TEST_FIXTURE_DIR "/two_qubit_bell.qke";

TEST(MQSSCIInterfacesTest, CompilesToOpenQASM2ForPlanqcBackend) {
  mqss::MQSSCompiler compiler;

  mqss::CompilerOptions opts;
  opts.optimization_level = 1;
  opts.result_type = "OpenQASM2";

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

} // namespace
