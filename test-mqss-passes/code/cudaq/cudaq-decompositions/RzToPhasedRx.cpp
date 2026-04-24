// Compile and run with:
// ```
// cudaq-quake RzToPhasedRx.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o RzToPhasedRx.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    z(q[0]);
    rz(1.3, q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
