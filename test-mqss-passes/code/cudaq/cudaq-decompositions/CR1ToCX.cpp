// Compile and run with:
// ```
// cudaq-quake CR1ToCX.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o CR1ToCX.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    z(q[0]);
    r1<cudaq::ctrl>(1.24, q[0], q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
