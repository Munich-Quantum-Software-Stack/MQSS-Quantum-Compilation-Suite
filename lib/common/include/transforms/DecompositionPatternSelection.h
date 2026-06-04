
#include "Decomposition.h"

#include <cassert>
#include <llvm/ADT/STLExtras.h>
#include <memory>
#include <optional>

//===----------------------------------------------------------------------===//
// Decomposition Graph for Pattern Selection
//===----------------------------------------------------------------------===//

/// DecompositionGraph constructs a hypergraph of decomposition patterns based
/// on pattern metadata and performs backward traversal to select patterns that
/// decompose to a basis.
///
/// Specifically, the decomposition graph is defined as a hypergraph in which
/// nodes are gate types and hyperedges are rewrite patterns connecting the
/// matched gate type to all newly inserted gate types.

namespace my_registry {
template <typename T> class RegisteredType {
public:
  using RegistryType = llvm::Registry<T>;
};
} // namespace my_registry

//===----------------------------------------------------------------------===//
// ConversionTarget and OperatorInfo, parsed from target basis strings such as
// ["x", "x(1)", "z"]
//===----------------------------------------------------------------------===//

struct OperatorInfo {
  StringRef name;
  std::size_t numControls = -1;

  // Default constructor — required by DecompositionPattern members + vector
  OperatorInfo() : name(), numControls(0) {}

  OperatorInfo(StringRef infoStr) : name(), numControls(0) {
    auto nameEnd = infoStr.find_first_of('(');
    name = infoStr.take_front(nameEnd);
    if (nameEnd < infoStr.size())
      infoStr = infoStr.drop_front(nameEnd);

    if (infoStr.consume_front("(")) {
      infoStr = infoStr.ltrim();
      if (infoStr.consume_front("n"))
        numControls = std::numeric_limits<std::size_t>::max();
      else
        infoStr.consumeInteger(10, numControls);
      assert(infoStr.trim().consume_front(")"));
    }
  }

  bool operator==(const OperatorInfo &other) const {
    return name == other.name && numControls == other.numControls;
  }
};

class DecompositionPatternType
    : public my_registry::RegisteredType<DecompositionPatternType> {
public:
  virtual ~DecompositionPatternType() = default;

  /// Get the source operation this pattern matches and decomposes.
  [[nodiscard]] virtual llvm::StringRef getSourceOp() const = 0;

  /// Get the target operations this pattern may produce
  virtual llvm::ArrayRef<llvm::StringRef> getTargetOps() const = 0;

  /// Get the name of the pattern.
  [[nodiscard]] virtual llvm::StringRef getPatternName() const = 0;

  /// Create a new instance of the pattern.
  virtual std::unique_ptr<mlir::RewritePattern>
  create(mlir::MLIRContext *context,
         mlir::PatternBenefit benefit = 1) const = 0;
};

struct DecompositionPattern {
  std::string name;
  OperatorInfo sourceOp;               // What to match
  std::vector<OperatorInfo> targetOps; // What to produce

  virtual void
  apply(MyModuleAnalysis &analysis) const = 0; // produce the "rewrite pattern"
};

namespace std {
template <> struct hash<OperatorInfo> {
  std::size_t operator()(const OperatorInfo &info) const {
    return llvm::hash_combine(info.name, info.numControls);
  }
};
} // namespace std

template <typename T>
std::size_t computeSetHash(const std::unordered_set<T> &set) {
  std::vector<std::size_t> hashes;
  for (const auto &elem : set) {
    hashes.push_back(std::hash<T>()(elem));
  }
  std::sort(hashes.begin(), hashes.end());
  return llvm::hash_combine_range(hashes.begin(), hashes.end());
}

struct BasisTarget : public ConversionTarget {

  BasisTarget(MLIRContext &context, ArrayRef<std::string> targetBasis)
      : ConversionTarget(context) {
    constexpr std::size_t unbounded = std::numeric_limits<std::size_t>::max();

    // Parse the list of target operations and build a set of legal operations
    for (const std::string &targetInfo : targetBasis)
      legalOperatorSet.emplace_back(targetInfo);
  }

  SmallVector<OperatorInfo, 8> legalOperatorSet;
};

class DecompositionGraph {
public:
  DecompositionGraph() = default;

  /// Construct a decomposition pattern graph from a collection of pattern
  /// types.
  DecompositionGraph(
      std::vector<std::unique_ptr<DecompositionPattern>> patterns,
      MyModuleAnalysis &analysis)
      : patternTypes(std::move(patterns)), analysis(analysis) {
    // Build the graph from pattern metadata
    for (const auto &pattern : patternTypes) {
      auto targetGates = pattern->targetOps;
      for (const auto &targetGate : targetGates)
        targetToPatterns[targetGate].push_back(pattern.get()->name);
    }
  }

  /// Create a DecompositionGraph from the registry entries.

  /// Return all patterns that have the given gate as one of their targets.
  ///
  /// @param gate The gate to find incoming patterns for
  /// @return A vector of pattern names (StringRef) whose targets include the
  /// given gate
  llvm::ArrayRef<std::string> incomingPatterns(const OperatorInfo &gate) const {
    static const llvm::SmallVector<std::string> empty;
    auto it = targetToPatterns.find(gate);
    return it == targetToPatterns.end() ? empty : it->second;
  }

  /// Select subset of patterns relevant to decomposing to the given basis
  /// gates.
  ///
  /// The result of the pattern selection are cached, so that successive calls
  /// with the same arguments will be O(1).
  ///
  /// @param patterns The pattern set to add the selected patterns to
  /// @param basisGates The basis gates to decompose to
  /// @param disabledPatterns The patterns to disable
  void selectAndApplyPatterns(const std::unordered_set<OperatorInfo> &basisGates,
                      const std::unordered_set<std::string> &disabledPatterns) {
    auto hashVal = llvm::hash_combine(computeSetHash(basisGates),
                                      computeSetHash(disabledPatterns));

    if (!patternSelectionCache.contains(hashVal)) {
      patternSelectionCache[hashVal] =
          computePatternSelection(basisGates, disabledPatterns);
    }

    for (const auto &patternName : patternSelectionCache[hashVal]) {
      const auto &pattern = getPatternType(patternName);
      assert(pattern && "Decomposition pattern not present for the gate");
      pattern->apply(analysis);
    }
  }

private:
  const DecompositionPattern *
  getPatternType(const std::string &patternName) const {
    for (const auto &p : patternTypes) {
      if (p->name == patternName)
        return p.get();
    }
    return nullptr;
  }

  /// Use Dijkstra's algorithm to compute the shortest decomposition path from
  /// every reachable gate type to the basis gates.
  ///
  /// This selects a unique decomposition path for each gate in the past of the
  /// basis gates in the decomposition graph, such that the number of patterns
  /// applied is minimized. `disabledPatterns` are ignored during the traversal
  /// and hence never selected.
  ///
  /// @param basisGates The set of basis gates to decompose to
  /// @param disabledPatterns The patterns to disable
  /// @return A vector of selected pattern names
  std::vector<std::string> computePatternSelection(
      const std::unordered_set<OperatorInfo> &basisGates,
      const std::unordered_set<std::string> &disabledPatterns) const {

    // An element in the priority queue of the Dijkstra algorithm (ordered by
    // smallest distance)
    struct GateDistancePair {
      OperatorInfo gate;
      std::size_t distance;
      std::optional<std::string> outgoingPattern;

      bool operator<(const GateDistancePair &other) const {
        // We want to order by smallest distance, so we invert the comparison
        return distance > other.distance;
      }
    };

    // Map: visited gate -> distance from the basis gates
    std::unordered_map<OperatorInfo, std::size_t> visitedGates;
    // The set of selected patterns to return
    std::vector<std::string> selectedPatterns;
    // Priority queue of gates to visit, sorted by smallest distance from the
    // basis gates
    std::priority_queue<GateDistancePair> gatesToVisit;

    // Initialize the priority queue with the basis gates
    for (const auto &gate : basisGates) {
      gatesToVisit.push({gate, 0, std::nullopt});
    }

    /// Compute the maximum distance from a pattern's targets to the basis
    /// gates.
    auto getPatternDist = [&](const auto &pattern) {
      auto targetGates = pattern->targetOps;
      std::vector<std::size_t> targetDistances;
      for (const auto &targetGate : targetGates) {
        if (visitedGates.count(targetGate)) {
          targetDistances.push_back(visitedGates.at(targetGate));
        } else {
          targetDistances.push_back(std::numeric_limits<std::size_t>::max());
        }
      }
      return *std::max_element(targetDistances.begin(), targetDistances.end());
    };

    while (!gatesToVisit.empty()) {
      auto [gate, dist, outgoingPattern] = gatesToVisit.top();
      gatesToVisit.pop();

      auto [_, success] = visitedGates.insert({gate, dist});
      if (!success) {
        // Gate already visited
        continue;
      }

      if (outgoingPattern.has_value()) {
        selectedPatterns.push_back(*outgoingPattern);
      }

      for (const auto &patternName : incomingPatterns(gate)) {
        if (disabledPatterns.contains(patternName)) {
          // Ignore disabled patterns
          continue;
        }
        const auto &pattern = getPatternType(patternName);
        std::size_t dist = getPatternDist(pattern);
        if (dist < std::numeric_limits<std::size_t>::max()) {
          gatesToVisit.push({pattern->sourceOp, dist + 1, patternName});
        }
      }
    }

    return selectedPatterns;
  }

  //===--------------------------------------------------------------------===//
  // Data structures for the graph definition
  //===--------------------------------------------------------------------===//

  /// All pattern types in the graph, keyed by pattern name.
  std::vector<std::unique_ptr<DecompositionPattern>> patternTypes;

  /// Map: target gate -> patterns that produce it
  std::unordered_map<OperatorInfo, SmallVector<std::string>> targetToPatterns;

  //===--------------------------------------------------------------------===//
  // Other data (cache)
  //===--------------------------------------------------------------------===//

  /// Cache for `selectPatterns`: hash of basis gates, disabled patterns,
  /// enabled patterns -> selected patterns
  std::unordered_map<std::size_t, std::vector<std::string>>
      patternSelectionCache;

  MyModuleAnalysis &analysis;
};
