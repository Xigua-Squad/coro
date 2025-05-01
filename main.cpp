#include "fmap.hpp"
#include "sync_wait.hpp"
#include "task.hpp"
#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
class P {
public:
  P() = default;
  P(P &&) { std::cout << "P&&\n"; }
};
auto get() { return P{}; }

void func(P a) {}

int main() {
  func(get());
  return 0;
}
