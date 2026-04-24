// Compile and run with:
// ```
// cudaq-quake SToPhasedRx.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o SToPhasedRx.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    x<cudaq::ctrl>(q[0], q[1]);
    s(q[0]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
