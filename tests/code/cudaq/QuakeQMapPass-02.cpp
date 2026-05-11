// Compile and run with:
// ```
// cudaq-quake test_QuakeQMapPass-02.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o
// test_QuakeQMapPass-02.qke
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
    x<cudaq::ctrl>(q[4], q[2]);
    x<cudaq::ctrl>(q[3], q[1]);
    x<cudaq::ctrl>(q[4], q[1]);
    rx(1.5, q[1]);
    ry(3.1416, q[2]);
    rz(2.25, q[3]);
    t(q[4]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<5>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
