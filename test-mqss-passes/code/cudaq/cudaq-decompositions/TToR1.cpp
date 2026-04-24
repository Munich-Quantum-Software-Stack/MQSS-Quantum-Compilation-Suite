// Compile and run with:
// ```
// cudaq-quake TToR1.cpp -o o.qke  &&
// cudaq-opt --canonicalize --unrolling-pipeline o.qke -o TToR1.qke
// ```

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    x(q[0]);
    t<cudaq::ctrl>(q[0], q[1]);
    mz(q);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
