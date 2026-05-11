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
    s<cudaq::adj>(q[0]);
    s(q[1]);
    z(q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = test<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
