#include <cudaq.h>
#include <fstream>
#include <iostream>

// Bell-state kernel: H on q0, then CNOT(q0, q1), measure both.
template <std::size_t N> struct bell {
  void operator()() __qpu__ {
    cudaq::qvector q(N);
    h(q[0]);
    x<cudaq::ctrl>(q[0], q[1]);
    mz(q[0]);
    mz(q[1]);
  }
};

// Rotation kernel: H + rz(pi/4) on q0, X on q1, then CNOT(q0, q1), measure
// both.
template <std::size_t N> struct rot {
  void operator()() __qpu__ {
    cudaq::qvector q(N);
    h(q[0]);
    rz(M_PI_4, q[0]);
    x(q[1]);
    x<cudaq::ctrl>(q[0], q[1]);
    mz(q[0]);
    mz(q[1]);
  }
};

int main() {
  auto bell_counts = cudaq::sample(bell<2>{});
  bell_counts.dump();

  auto rot_counts = cudaq::sample(rot<2>{});
  rot_counts.dump();

  return 0;
}
