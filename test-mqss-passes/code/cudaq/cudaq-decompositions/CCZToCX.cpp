// Compile and run with:
// ```
// cudaq-quake CCZToCX.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o CCZToCX.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    y(q[0]);
    z<cudaq::ctrl>(q[0], q[1], q[2]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<3>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
