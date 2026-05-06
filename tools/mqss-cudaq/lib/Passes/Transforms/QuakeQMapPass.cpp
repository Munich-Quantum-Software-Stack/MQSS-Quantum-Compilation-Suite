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
  QuakeQMapPass(Architecture architecture, Configuration settings)
  This pass performs the mapping of Quantum kernels defined in MLIR/Quake.
  The mapper used is MQT-QMAP. As inputs, it will receive the target
architecture where Quantum kernels must execute and the settings to configure
que MQT-QMAP mapper. The mapper modifies the input Quantum kernels to be
correctly mapped to a given quantum device (Architecture) according to the
mapping configurations.

******************************************************************************/

// #include "Support/CodeGen/Quake.hpp"
#include "mlir/Dialect/SCF/IR/SCF.h"

#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"
#include "SemanticExtractLayer/QuakeExtractor.h"

#include "llvm/Support/raw_ostream.h"
// #include "qdmi.h"
#include "sc/heuristic/HeuristicMapper.hpp"

#include "MQSSCUDAQPasses/Transforms.hpp"

namespace mqss_cudaq::opt {
#define GEN_PASS_CLASSES
#include "MQSSCUDAQPasses/Transforms.h.inc"

} // namespace mqss_cudaq::opt


using namespace mlir;

struct PipelineConfig {
  Architecture arch;
  Configuration settings;

  static PipelineConfig defaults() {
    PipelineConfig config;
    // Defining test architecture
    /*
        3
       / \
      4   2
      |   |
      0---1
    */
    const CouplingMap cm = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
                            {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
    config.arch.loadCouplingMap(5, cm);
    // Defining the settings of the mqt-mapper
    config.settings.heuristic = Heuristic::GateCountMaxDistance;
    config.settings.layering = Layering::DisjointQubits;
    config.settings.initialLayout = InitialLayout::Identity;
    config.settings.preMappingOptimizations = false;
    config.settings.postMappingOptimizations = false;
    config.settings.lookaheadHeuristic = LookaheadHeuristic::None;
    config.settings.debug = false;
    config.settings.addMeasurementsToMappedCircuit = true;
    return config;
  };

  static PipelineConfig fromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open())
      throw std::runtime_error("Could not open config file: " + path);

    nlohmann::json j;
    file >> j;

    PipelineConfig config;

    // --- Architecture ---
    const auto &arch = j.at("architecture");
    CouplingMap cm;
    for (const auto &pair : arch.at("coupling_map"))
      cm.insert({pair[0].get<int>(), pair[1].get<int>()});
    config.arch.loadCouplingMap(arch.at("num_qubits").get<int>(), cm);

    // --- Mapper Settings ---
    const auto &ms = j.at("mapper_settings");
    config.settings.heuristic = heuristicFromString(ms.at("heuristic"));
    config.settings.layering = layeringFromString(ms.at("layering"));
    config.settings.initialLayout =
        initialLayoutFromString(ms.at("initial_layout"));
    config.settings.preMappingOptimizations =
        ms.at("pre_mapping_optimizations").get<bool>();
    config.settings.postMappingOptimizations =
        ms.at("post_mapping_optimizations").get<bool>();
    config.settings.lookaheadHeuristic =
        lookaheadFromString(ms.at("lookahead_heuristic"));
    config.settings.debug = ms.at("debug").get<bool>();
    config.settings.addMeasurementsToMappedCircuit =
        ms.at("add_measurements_to_mapped_circuit").get<bool>();

    return config;
  }

private:
  // -- String -> Enum helpers --
  static Heuristic heuristicFromString(const std::string &s) {
    if (s == "GateCountMaxDistance")
      return Heuristic::GateCountMaxDistance;
    if (s == "GateCountSumDistance")
      return Heuristic::GateCountSumDistance;
    // ... other variants
    throw std::invalid_argument("Unknown heuristic: " + s);
  }

  static Layering layeringFromString(const std::string &s) {
    if (s == "DisjointQubits")
      return Layering::DisjointQubits;
    if (s == "OddGates")
      return Layering::OddGates;
    // ... other variants
    throw std::invalid_argument("Unknown layering: " + s);
  }

  static InitialLayout initialLayoutFromString(const std::string &s) {
    if (s == "Identity")
      return InitialLayout::Identity;
    if (s == "Static")
      return InitialLayout::Static;
    if (s == "Dynamic")
      return InitialLayout::Dynamic;
    // ... other variants
    throw std::invalid_argument("Unknown initial layout: " + s);
  }

  static LookaheadHeuristic lookaheadFromString(const std::string &s) {
    if (s == "None")
      return LookaheadHeuristic::None;
    if (s == "GateCountMaxDistance")
      return LookaheadHeuristic::GateCountMaxDistance;
    // ... other variants
    throw std::invalid_argument("Unknown lookahead heuristic: " + s);
  }
};

int64_t extractIndexFromQuakeExtractRefOp(Operation *op) {
  if (auto extractRefOp = llvm::dyn_cast<quake::ExtractRefOp>(op)) {
    auto rawIndexAttr =
        extractRefOp->getAttrOfType<mlir::IntegerAttr>("rawIndex");
    return rawIndexAttr.getInt();
  }
  return -1;
}

double extractDoubleArgumentValue(Operation *op) {
  if (auto constantOp = dyn_cast<mlir::arith::ConstantOp>(op))
    if (auto floatAttr = constantOp.getValue().dyn_cast<mlir::FloatAttr>())
      return static_cast<float>(floatAttr.getValueAsDouble());
  return -1.0;
}

// function to get the number of qubits in a given quantum kernel
int getNumberOfQubits(func::FuncOp circuit) {
  int numQubits = 0;
  circuit.walk([&](quake::AllocaOp allocOp) {
    if (auto qrefType = allocOp.getType().dyn_cast<quake::RefType>()) {
      numQubits += 1;
    } else if (auto qvecType = allocOp.getType().dyn_cast<quake::VeqType>()) {
      numQubits += qvecType.getSize();
    }
  });
  return numQubits;
}

// Function to get the number of classical bits allocated in a given
// quantum kernel, it also stores information of the qiubit position
int getNumberOfClassicalBits(
    func::FuncOp circuit, std::map<int, int> &measurements) {
  int numBits = 0;
  circuit.walk([&](mlir::Operation *op) {
    if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op)) {
      for (auto operand : op->getOperands()) {
        if (operand.getType()
                .isa<quake::RefType>()) { // Check if it's qubit reference
          int qubitIndex =
              extractIndexFromQuakeExtractRefOp(operand.getDefiningOp());
          assert(qubitIndex != -1 && "Non valid qubit index for measurement!");
          measurements[qubitIndex] = numBits;
          numBits += 1;
        } else if (operand.getType().isa<quake::VeqType>()) {
          auto qvecType = operand.getType().dyn_cast<quake::VeqType>();
          numBits += qvecType.getSize();
          for (int i = 0; i < numBits; i++) {
            measurements[i] = i;
          }
        }
      }
    }
  });
  return numBits;
}


// loading rotation gates
void loadRotationGatesToQC(Operation *op, qc::QuantumComputation &qc) {
  if (isa<quake::RxOp>(op) || isa<quake::RyOp>(op) || isa<quake::RzOp>(op)) {
    int qubit = -1;
    double angle = -1.0;
    assert(op->getOperands().size() == 2 && "ill-formed rotation gate!");
    Value operand1 = op->getOperands()[0];
    angle = extractDoubleArgumentValue(operand1.getDefiningOp());
    Value operand2 = op->getOperands()[1];
    qubit = extractIndexFromQuakeExtractRefOp(
        operand2.getDefiningOp());
#ifdef DEBUG
    llvm::errs() << "Operation ";
    op->print(llvm::errs());
    llvm::errs() << "\n";
    llvm::errs() << "\tRotation with angle " << angle << " on qubit " << qubit
                 << "\n";
#endif
    assert(!(angle == -1.0 || qubit == -1) && "ill-formed rotation gate!");
    if (isa<quake::RxOp>(op))
      qc.rx(angle, qubit);
    if (isa<quake::RyOp>(op))
      qc.ry(angle, qubit);
    if (isa<quake::RzOp>(op))
      qc.rz(angle, qubit);
  }
}
// loading X, Y , Z
// two bits X,Y and Z refers to controlled Cx, Cy, and Cz
// single bits are just x,y,and z
void loadXYZGatesToQC(Operation *op, qc::QuantumComputation &qc) {
  if (isa<quake::XOp>(op) || isa<quake::YOp>(op) || isa<quake::ZOp>(op)) {
    // controlled operations
    if (op->getOperands().size() == 2) {
      int qubit_ctrl, qubit_target;
      Value operand1 = op->getOperands()[0];
      qubit_ctrl = extractIndexFromQuakeExtractRefOp(
          operand1.getDefiningOp());
      Value operand2 = op->getOperands()[1];
      qubit_target = extractIndexFromQuakeExtractRefOp(
          operand2.getDefiningOp());
#ifdef DEBUG
      llvm::errs() << "Operation ";
      op->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "\tqubit_ctrl " << qubit_ctrl << " qubit_target "
                   << qubit_target << "\n";
#endif
      assert(!(qubit_ctrl == -1 || qubit_target == -1) &&
             "ill-formed controlled gate!");
      if (isa<quake::XOp>(op))
        qc.cx(qubit_ctrl, qubit_target);
      if (isa<quake::YOp>(op))
        qc.cy(qubit_ctrl, qubit_target);
      if (isa<quake::ZOp>(op))
        qc.cz(qubit_ctrl, qubit_target);
    }
    // single qubit operations
    if (op->getOperands().size() == 1) {
      Value operand1 = op->getOperands()[0];
      int qubit =extractIndexFromQuakeExtractRefOp(
          operand1.getDefiningOp());
#ifdef DEBUG
      llvm::errs() << "Operation ";
      op->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "\tSingle qubit operation on qubit " << qubit << "\n";
#endif
      assert(qubit != -1 && "ill-formed single gate X, Y and Z!");
      if (isa<quake::XOp>(op))
        qc.x(qubit);
      if (isa<quake::YOp>(op))
        qc.y(qubit);
      if (isa<quake::ZOp>(op))
        qc.z(qubit);
    }
  }
}
// loading S,T,H single qubit gates
void loadSTHGatesToQC(Operation *op, qc::QuantumComputation &qc) {
  if (isa<quake::SOp>(op) || isa<quake::TOp>(op) || isa<quake::HOp>(op)) {
    int qubit_ctrl, qubit_target;
    // single qubit operations
    if (op->getOperands().size() == 1) {
      Value operand1 = op->getOperands()[0];
      int qubit =extractIndexFromQuakeExtractRefOp(
          operand1.getDefiningOp());
#ifdef DEBUG
      llvm::errs() << "Operation ";
      op->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "\tSingle qubit operation on qubit " << qubit << "\n";
#endif
      assert(qubit != -1 && "ill-formed single gate, S, T or H !");
      if (isa<quake::SOp>(op))
        qc.s(qubit);
      if (isa<quake::TOp>(op))
        qc.t(qubit);
      if (isa<quake::HOp>(op))
        qc.h(qubit);
    }
  }
}
// loading measurements
void loadMeasurementsToQC(Operation *op, qc::QuantumComputation &qc,
                          std::map<int, int> measurements) {
  int qubit = -1, result = -1;
  if (isa<quake::MxOp>(op) || isa<quake::MyOp>(op) || isa<quake::MzOp>(op)) {
#ifdef DEBUG
    llvm::errs() << "Operation ";
    op->print(llvm::errs());
    llvm::errs() << "\n";
#endif
    assert(op->getOperands().size() == 1 && "ill-formed measurement gate!");
    Value operand = op->getOperands()[0];
    if (operand.getType().isa<quake::RefType>()) {
      int qubitIndex =extractIndexFromQuakeExtractRefOp(
          operand.getDefiningOp());
      assert(qubitIndex != -1 && "Non valid qubit index for measurement!");
      qc.measure(static_cast<qc::Qubit>(qubitIndex),
                 measurements.at(qubitIndex));
#ifdef DEBUG
      llvm::errs() << "\tMeasurement on qubit index " << qubitIndex << "\n";
#endif
    } else if (operand.getType().isa<quake::VeqType>()) {
      auto qvecType = operand.getType().dyn_cast<quake::VeqType>();
      int nQubits = qvecType.getSize();
      qc.measureAll();
#ifdef DEBUG
      llvm::errs() << "\tMeasurement on vector of size " << nQubits << "\n";
#endif
    }
  }
}

namespace {

class QuakeQMap : public mqss_cudaq::opt::QuakeQMapPassBase<class QuakeQMap> {

public:

  // MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(QuakeQMap)

  // QuakeQMap(Architecture arch, const Configuration &cfg)
  //       : architecture(std::move(arch)), settings(std::move(cfg)) {}

  //   // MLIR requires passes to be copyable for cloning
  //   QuakeQMap(const QuakeQMap &other) = default;

  // llvm::StringRef getArgument() const override { return "quake-to-qmap-pass"; }
  // llvm::StringRef getDescription() const override {
  //   return "Pass that maps a given quake module respecting the constraints of "
  //          "a given quantum device, using mqt-qmap tool";
  // }

  void performMapping(mlir::func::FuncOp circuit, Architecture architecture, Configuration settings) {
    StringRef funcName = circuit.getName();
    if (!(funcName.find(std::string(CUDAQ_PREFIX_FUNCTION)) !=
          std::string::npos))
      return; // do nothing if the function is not cudaq kernel

    std::map<int, int> measurements; // key: qubit, value register index
    int numQubits = getNumberOfQubits(circuit);
    int numBits = getNumberOfClassicalBits(circuit, measurements);
#ifdef DEBUG
    llvm::outs() << "Kernel name: " << funcName << "\n";
    llvm::errs() << "Number of input qubits " << numQubits << "\n";
    llvm::errs() << "Number of output bits " << numBits << "\n";
#endif
    // Defining the mqt-qmap input object
    auto qc = qc::QuantumComputation(numQubits, numBits);
    // Traversing input QUAKE MLIR
    circuit.walk([&](mlir::Operation *op) {
      // TODO: Assumed at the moment to work only on a single qubit
      loadRotationGatesToQC(op, qc);
      // TODO: Cover only the case of single qubit and 2 qubit controlled
      // operations
      loadXYZGatesToQC(op, qc);
      // TODO: Assumed at the moment to work only on a single qubit
      loadSTHGatesToQC(op, qc);
      loadMeasurementsToQC(op, qc, measurements);
    });
#ifdef DEBUG
    // Printing the parsed mlir quantum kernel
    llvm::errs() << "Dumping QC:\n";
    qc.print(std::cout);
#endif
    // Map the circuit
    const auto mapper = std::make_unique<HeuristicMapper>(qc, architecture);
    mapper->map(settings);
    // TODO: There should be other way to get the mapped circuit.
    //       I do not like to down the mapped circuit to QASM and
    //        then back to qc
    auto qcMapped = qc::QuantumComputation();
    qcMapped = mapper->moveMappedCircuit();
    // cleaning the mlir::funcOp corresponding to the quake circuit
    for (auto &block : circuit.getBody()) {
      block.clear(); // Clears all operations in the current block
    }
    OpBuilder builder(&circuit.getBody());
    Location loc = circuit.getLoc();
    // allocate the qubits
    Value qubits = builder.create<quake::AllocaOp>(
        circuit.getLoc(), quake::VeqType::get(builder.getContext(), numQubits));
    // then traverse the mapped QuantumComputation and annotate it in the
    // mlir func
    for (const auto &op : qcMapped) {
      if (op->getType() == qc::Barrier)
        continue;
      auto &targets = op->getTargets();
      auto &controls = op->getControls();
      auto parameter = op->getParameter();
      // defining the list of controls, targets and parameters
      SmallVector<Value> parameterValues = {};
      SmallVector<Value> controlValues = {};
      SmallVector<Value> targetValues = {};
      // get the targets
      for (int i = 0; i < targets.size(); i++) {
        auto targetRef =
            builder.create<quake::ExtractRefOp>(loc, qubits, targets[i]);
        targetValues.push_back(targetRef);
      }
      // get the controls
      for (auto q : controls) {
        auto controlRef =
            builder.create<quake::ExtractRefOp>(loc, qubits, q.qubit);
        controlValues.push_back(controlRef);
      }
      // get the parameters
      for (auto p : parameter) {
        // TODO: Apparently all the parameters are floats in QC, may be the case
        //       this is not always true
        llvm::APFloat constantValue(p);
        // Define the type as f64.
        auto floatType = builder.getF64Type();
        auto constantOp = builder.create<arith::ConstantFloatOp>(
            loc, constantValue, floatType);
        parameterValues.push_back(constantOp);
      }
      switch (op->getType()) {
      case qc::X:
        builder.create<quake::XOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::Y:
        builder.create<quake::YOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::Z:
        builder.create<quake::ZOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::RX:
        builder.create<quake::RxOp>(loc, parameterValues, controlValues,
                                    targetValues);
        break;
      case qc::RY:
        builder.create<quake::RyOp>(loc, parameterValues, controlValues,
                                    targetValues);
        break;
      case qc::RZ:
        builder.create<quake::RzOp>(loc, parameterValues, controlValues,
                                    targetValues);
        break;
      case qc::SWAP:
        builder.create<quake::SwapOp>(loc, parameterValues, controlValues,
                                      targetValues);
        break;
      case qc::H:
        builder.create<quake::HOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::S:
        builder.create<quake::SOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::T:
        builder.create<quake::TOp>(loc, parameterValues, controlValues,
                                   targetValues);
        break;
      case qc::Measure:
        Type measTy = quake::MeasureType::get(builder.getContext());
        builder.create<quake::MzOp>(loc, measTy, targetValues).getMeasOut();
        break;
      }
    }
    builder.create<func::ReturnOp>(circuit.getLoc());
#ifdef DEBUG
    std::cout << "Dumping QC after mapping:\n";
    qcMapped.print(std::cout);
#endif
  }

  void runOnOperation() override {
    // Getting the function
    auto module = getOperation();

    auto config = PipelineConfig::fromFile("/");
    auto architecture=config.arch;
    auto settings = config.settings;
    module.walk([&](Operation *op){
        if(auto funcOp = dyn_cast<mlir::func::FuncOp>(op))
            performMapping(funcOp, architecture, settings);
    });
    // Get the function name
    
  }
};
} // namespace

std::unique_ptr<mlir::Pass> mqss_cudaq::opt::QuakeQMapPass() {
  return std::make_unique<QuakeQMap>();
}
