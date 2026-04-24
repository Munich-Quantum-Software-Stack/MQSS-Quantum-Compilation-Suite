// Compile and run with:
// ```
// cudaq-quake U3ToRotations.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o U3ToRotations.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    z(q[0]);
    u3(M_PI, M_PI, M_PI_2, q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
