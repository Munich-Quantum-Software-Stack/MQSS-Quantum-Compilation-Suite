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
    x(q[0]);
    h(q[0]);
    h(q[1]);
    x(q[1]);
    mz(q[0]);
    mz(q[1]);
  }
};

template <std::size_t N> struct test1 {
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

template <std::size_t N> struct test2 {
  auto operator()() __qpu__ {

    // Compile-time sized array like std::array
    cudaq::qarray<N> q;
    z(q[0]);
    h(q[0]);
    h(q[1]);
    z(q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = test<2>{};
  auto counts = cudaq::sample(kernel);
  auto kernel1 = test1<2>{};
  auto counts1 = cudaq::sample(kernel1);
  auto kernel2 = test2<2>{};
  auto counts2 = cudaq::sample(kernel2);
  counts.dump();
  counts1.dump();
  counts2.dump();
  return 0;
}
