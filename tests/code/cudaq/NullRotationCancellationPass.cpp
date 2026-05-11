// Compile and run with:
// ```
// cudaq-quake CommuteCNotZPass.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o CommuteCNotZPass.qke
// ```

#include <cudaq.h>
#include <fstream>
#include <iostream>

// Define a CUDA-Q kernel that is fully specified
// at compile time via templates.
template <std::size_t N> struct test {
  auto operator()() __qpu__ {

    // Compile-time sized array like std::array
    cudaq::qarray<N> q;
    rx(0.0, q[1]);
    ry(2.0, q[0]);
    rz(0.0, q[0]);

    rx(11.3, q[1]);
    ry(0.0, q[0]);
    rz(6.2832, q[1]);

    mz(q);
  }
};

int main() {
  auto kernel = test<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
