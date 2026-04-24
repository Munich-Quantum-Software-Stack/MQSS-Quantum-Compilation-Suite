// Compile and run with:
// ```
// cudaq-quake QuakeToTikzPass.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o QuakeToTikzPass.qke
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
    y(q[0]);
    h(q[0]);
    h(q[1]);
    y(q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = test<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
