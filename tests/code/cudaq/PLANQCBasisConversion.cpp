#include <cudaq.h>
#include <fstream>
#include <iostream>

// Bell-state kernel: H on q0, then CNOT(q0, q1), measure both.
template <std::size_t N>
struct bell {
  void operator()() __qpu__ {
    cudaq::qvector q(N);
    h(q[0]);
    x<cudaq::ctrl>(q[0], q[1]);
    mz(q[0]);
    mz(q[1]);
  }
};

int main() {
  auto bell_counts = cudaq::sample(bell<2>{});
  bell_counts.dump();
  return 0;
}