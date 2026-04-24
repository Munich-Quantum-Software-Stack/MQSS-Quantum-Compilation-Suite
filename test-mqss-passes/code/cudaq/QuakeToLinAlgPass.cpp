// Compile and run with:
// ```
// cudaq-quake test_QuakeQMapPass-01.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o
// test_QuakeQMapPass-01.qke
// ```

#include <cudaq.h>
#include <fstream>
#include <iostream>

// Define a CUDA-Q kernel that is fully specified
// at compile time via templates.
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    // Compile-time sized array like std::array
    cudaq::qarray<N> q;
    h(q[0]);
    x(q[0]);
    h(q[0]);
    h(q[1]);
    x<cudaq::ctrl>(q[0], q[1]);
    h(q[0]);
    y(q[2]);
    r1(3.1416, q[2]);
    ry(1.234, q[2]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<3>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
