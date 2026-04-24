// Compile and run with:
// ```
// cudaq-quake R1ToRz.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o R1ToRz.qke
// ```

#include <cudaq.h>
#include <numbers>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    x<cudaq::ctrl>(q[0], q[1]);
    r1(3.1416, q[0]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
