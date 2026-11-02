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

#include "Translation/QASM2ToQuake.h"

#include "cudaq/Optimizer/Dialect/CC/CCDialect.h"
#include "cudaq/Optimizer/Dialect/CC/CCTypes.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeDialect.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeOps.h"
#include "cudaq/Optimizer/Dialect/Quake/QuakeTypes.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Location.h"
#include "mlir/Tools/mlir-translate/Translation.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <cctype>
#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace mlir;
namespace quake = cudaq::quake;

namespace {

// Name of the kernel emitted for the QASM2 program. CUDA-Q's bridge uses the
// same `__nvqpp__mlirgen__` prefix for kernels it generates.
constexpr llvm::StringLiteral kKernelName = "__nvqpp__mlirgen__qasm2_kernel";

// A `;`-terminated statement with comments stripped, and where it started.
struct Statement {
  std::string text;
  unsigned line;
  unsigned col;
};

// A register reference: `name` (whole register) or `name[index]`.
struct Operand {
  llvm::StringRef name;
  std::optional<unsigned> index;
};

// A declared register, laid out at [offset, offset + size) of its kind's
// flat index space (qubits for qreg, classical bits for creg).
struct Register {
  unsigned offset;
  unsigned size;
};

// Supported gates: how many leading operands are controls, and how many
// targets follow.
struct GateInfo {
  unsigned numControls;
  unsigned numTargets;
  std::function<void(OpBuilder &, Location, ValueRange, ValueRange)> emit;
};

template <typename GateOp>
GateInfo gate(unsigned numControls, bool isAdj = false) {
  return {numControls, 1,
          [isAdj](OpBuilder &b, Location loc, ValueRange controls,
                  ValueRange targets) {
            b.create<GateOp>(loc, isAdj, ValueRange{}, controls, targets);
          }};
}

const llvm::StringMap<GateInfo> &supportedGates() {
  static const llvm::StringMap<GateInfo> gates = {
      {"h", gate<quake::HOp>(0)},  {"x", gate<quake::XOp>(0)},
      {"y", gate<quake::YOp>(0)},  {"z", gate<quake::ZOp>(0)},
      {"s", gate<quake::SOp>(0)},  {"sdg", gate<quake::SOp>(0, /*isAdj=*/true)},
      {"t", gate<quake::TOp>(0)},  {"tdg", gate<quake::TOp>(0, /*isAdj=*/true)},
      {"cx", gate<quake::XOp>(1)}, {"cz", gate<quake::ZOp>(1)},
  };
  return gates;
}

// Splits `source` into `;`-terminated statements, dropping `//` comments.
// Returns false (after emitting a diagnostic) on trailing text without `;`.
bool splitStatements(llvm::StringRef source, MLIRContext &context,
                     std::vector<Statement> &statements) {
  Statement current{"", 0, 0};
  unsigned line = 1, col = 1;
  auto startNewStatement = [&] { current = Statement{"", 0, 0}; };

  for (size_t i = 0; i < source.size(); ++i) {
    char c = source[i];
    if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
      while (i < source.size() && source[i] != '\n')
        ++i;
      c = '\n'; // fall through so the newline updates line/col below
    }
    if (c == ';') {
      statements.push_back(current);
      startNewStatement();
    } else if (!std::isspace(static_cast<unsigned char>(c)) ||
               !current.text.empty()) {
      if (current.text.empty()) {
        current.line = line;
        current.col = col;
      }
      current.text += c;
    }
    if (c == '\n') {
      ++line;
      col = 1;
    } else {
      ++col;
    }
  }

  if (!llvm::StringRef(current.text).trim().empty()) {
    mlir::emitError(
        FileLineColLoc::get(&context, "<qasm2>", current.line, current.col))
        << "QASM2ToQuake: missing ';' at end of statement";
    return false;
  }
  return true;
}

class QASM2ToQuakeTranslator {
public:
  explicit QASM2ToQuakeTranslator(MLIRContext &context)
      : context(context), builder(&context), loc(UnknownLoc::get(&context)) {}

  OwningOpRef<ModuleOp> translate(llvm::StringRef source) {
    std::vector<Statement> statements;
    if (!splitStatements(source, context, statements))
      return nullptr;

    OwningOpRef<ModuleOp> module = ModuleOp::create(loc);
    builder.setInsertionPointToStart(module->getBody());
    auto kernel = builder.create<func::FuncOp>(loc, kKernelName,
                                               builder.getFunctionType({}, {}));
    kernel->setAttr("cudaq-entrypoint", builder.getUnitAttr());
    kernel->setAttr("cudaq-kernel", builder.getUnitAttr());
    builder.setInsertionPointToStart(kernel.addEntryBlock());

    for (const Statement &stmt : statements) {
      loc = FileLineColLoc::get(&context, "<qasm2>", stmt.line, stmt.col);
      if (failed(translateStatement(llvm::StringRef(stmt.text).trim())))
        return nullptr;
    }

    loc = UnknownLoc::get(&context);
    builder.create<func::ReturnOp>(loc);
    return module;
  }

private:
  MLIRContext &context;
  OpBuilder builder;
  Location loc;

  llvm::StringMap<Register> qregs, cregs;
  unsigned numQubits = 0;
  Value veq;                 // the single quake.alloca holding every qubit
  std::vector<Value> qubits; // one quake.extract_ref per qubit
  bool allocated = false;

  // Parses `name` or `name[index]`.
  std::optional<Operand> parseOperand(llvm::StringRef text) {
    text = text.trim();
    size_t open = text.find('[');
    if (open == llvm::StringRef::npos)
      return text.empty() ? std::nullopt
                          : std::optional<Operand>(Operand{text, {}});
    if (!text.ends_with("]"))
      return std::nullopt;
    unsigned index;
    if (text.slice(open + 1, text.size() - 1).trim().getAsInteger(10, index))
      return std::nullopt;
    return Operand{text.take_front(open).trim(), index};
  }

  // Emits the single quake.alloca (sized from the qregs declared so far) and
  // one quake.extract_ref per qubit. Done lazily so declarations may be spread
  // over several statements; they must still precede the first operation.
  void ensureAllocated() {
    if (allocated)
      return;
    allocated = true;
    veq = builder.create<quake::AllocaOp>(
        loc, quake::VeqType::get(builder.getContext(), numQubits));
    for (unsigned i = 0; i < numQubits; ++i)
      qubits.push_back(builder.create<quake::ExtractRefOp>(loc, veq, i));
  }

  LogicalResult declareRegister(llvm::StringRef args, bool isQuantum) {
    auto operand = parseOperand(args);
    if (!operand || !operand->index || *operand->index == 0)
      return mlir::emitError(loc)
             << "QASM2ToQuake: expected '" << (isQuantum ? "qreg" : "creg")
             << " name[size];' with size > 0";
    if (isQuantum && allocated)
      return mlir::emitError(loc)
             << "QASM2ToQuake: qreg must be declared before the first gate";
    auto &regs = isQuantum ? qregs : cregs;
    if (qregs.contains(operand->name) || cregs.contains(operand->name))
      return mlir::emitError(loc) << "QASM2ToQuake: register '" << operand->name
                                  << "' is already declared";
    unsigned offset = 0;
    for (auto &entry : regs)
      offset += entry.second.size;
    regs[operand->name] = {offset, *operand->index};
    if (isQuantum)
      numQubits += *operand->index;
    return success();
  }

  // Resolves `q[i]` to its extracted qubit.
  Value lookupQubit(const Operand &operand) {
    auto it = qregs.find(operand.name);
    if (it == qregs.end()) {
      emitError(loc) << "QASM2ToQuake: unknown qreg '" << operand.name << "'";
      return nullptr;
    }
    if (!operand.index) {
      emitError(loc) << "QASM2ToQuake: gates on a whole register ('"
                     << operand.name << "') are not supported; index a qubit";
      return nullptr;
    }
    if (*operand.index >= it->second.size) {
      emitError(loc) << "QASM2ToQuake: index " << *operand.index
                     << " out of range for qreg '" << operand.name << "'";
      return nullptr;
    }
    return qubits[it->second.offset + *operand.index];
  }

  LogicalResult translateGate(llvm::StringRef name, llvm::StringRef args) {
    auto it = supportedGates().find(name);
    if (it == supportedGates().end())
      return emitError(loc)
             << "QASM2ToQuake: unsupported gate '" << name << "'";
    const GateInfo &info = it->second;

    llvm::SmallVector<llvm::StringRef> operandTexts;
    args.split(operandTexts, ',');
    if (operandTexts.size() != info.numControls + info.numTargets)
      return emitError(loc)
             << "QASM2ToQuake: gate '" << name << "' expects "
             << info.numControls + info.numTargets << " qubit operand(s)";

    ensureAllocated();
    llvm::SmallVector<Value> controls, targets;
    for (auto [i, text] : llvm::enumerate(operandTexts)) {
      auto operand = parseOperand(text);
      if (!operand)
        return emitError(loc)
               << "QASM2ToQuake: malformed operand '" << text.trim() << "'";
      Value qubit = lookupQubit(*operand);
      if (!qubit)
        return failure();
      (i < info.numControls ? controls : targets).push_back(qubit);
    }
    info.emit(builder, loc, controls, targets);
    return success();
  }

  // `measure q[i] -> c[j]` or `measure q -> c` (same-size registers).
  LogicalResult translateMeasure(llvm::StringRef args) {
    auto [lhs, rhs] = args.split("->");
    auto src = parseOperand(lhs);
    auto dst = parseOperand(rhs);
    if (!src || !dst || rhs.empty())
      return emitError(loc) << "QASM2ToQuake: expected 'measure qubit -> bit;'";

    auto qreg = qregs.find(src->name);
    auto creg = cregs.find(dst->name);
    if (qreg == qregs.end() || creg == cregs.end())
      return emitError(loc) << "QASM2ToQuake: unknown register in measure";
    if (src->index.has_value() != dst->index.has_value())
      return emitError(loc) << "QASM2ToQuake: measure operands must both be "
                               "indexed or both be whole registers";

    ensureAllocated();
    Type measTy = quake::MeasureType::get(builder.getContext());

    if (src->index) {
      if (*src->index >= qreg->second.size || *dst->index >= creg->second.size)
        return emitError(loc) << "QASM2ToQuake: measure index out of range";
      builder.create<quake::MzOp>(
          loc, measTy, ValueRange{qubits[qreg->second.offset + *src->index]});
      return success();
    }
    if (qreg->second.size != creg->second.size)
      return emitError(loc) << "QASM2ToQuake: measure registers differ in size";

    // `measure q -> c;` over a register that is the whole allocation becomes
    // a single veq measurement, as CUDA-Q emits for a register-wide mz.
    if (qreg->second.offset == 0 && qreg->second.size == numQubits) {
      Type vecTy = cudaq::cc::StdvecType::get(builder.getContext(), measTy);
      builder.create<quake::MzOp>(loc, vecTy, ValueRange{veq});
      return success();
    }
    // A register that is only part of the allocation is measured qubit by
    // qubit instead.
    for (unsigned i = 0; i < qreg->second.size; ++i)
      builder.create<quake::MzOp>(loc, measTy,
                                  ValueRange{qubits[qreg->second.offset + i]});
    return success();
  }

  LogicalResult translateStatement(llvm::StringRef text) {
    // Leading keyword/gate name; whatever follows is its argument list.
    size_t end = 0;
    while (end < text.size() &&
           (std::isalnum(static_cast<unsigned char>(text[end])) ||
            text[end] == '_'))
      ++end;
    llvm::StringRef keyword = text.take_front(end);
    llvm::StringRef rest = text.drop_front(end).trim();

    if (keyword == "OPENQASM") {
      if (!rest.starts_with("2"))
        return emitError(loc) << "QASM2ToQuake: only OPENQASM 2.x is supported";
      return success();
    }
    if (keyword == "include")
      return success(); // qelib1.inc's gates are built in
    if (keyword == "qreg")
      return declareRegister(rest, /*isQuantum=*/true);
    if (keyword == "creg")
      return declareRegister(rest, /*isQuantum=*/false);
    if (keyword == "measure")
      return translateMeasure(rest);
    return translateGate(keyword, rest);
  }
};

} // namespace

mlir::OwningOpRef<mlir::ModuleOp>
mqss::mqssci::translation::translateQASM2ToQuake(llvm::StringRef qasm,
                                                 mlir::MLIRContext &context) {
  context.loadDialect<func::FuncDialect, cudaq::cc::CCDialect,
                      quake::QuakeDialect>();
  return QASM2ToQuakeTranslator(context).translate(qasm);
}

void mqss::mqssci::translation::registerQASM2ToQuakeTranslation() {
  static mlir::TranslateToMLIRRegistration registration(
      "import-qasm2", "Import OpenQASM 2 into the CUDA-Q Quake dialect",
      mlir::TranslateStringRefToMLIRFunction(
          [](llvm::StringRef source,
             mlir::MLIRContext *context) -> OwningOpRef<Operation *> {
            return OwningOpRef<Operation *>(
                translateQASM2ToQuake(source, *context).release());
          }),
      [](mlir::DialectRegistry &registry) {
        registry.insert<func::FuncDialect, cudaq::cc::CCDialect,
                        quake::QuakeDialect>();
      });
}
