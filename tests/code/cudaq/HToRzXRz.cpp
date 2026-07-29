

#include <cudaq.h>
template <std::size_t N> struct ghz {
  auto operator()() __qpu__ {
    cudaq::qvector q(N);
    x<cudaq::ctrl>(q[0], q[1]);
    h(q[0]);
    mz(q[0]);
    mz(q[1]);
  }
};

int main() {
  auto kernel = ghz<2>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}
