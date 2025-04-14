#include <iostream>
int global_val = 89;
class A {
public:
  int &func() & {
    std::cout << "& func() \n";
    return global_val;
  }
  int func() && {
    std::cout << "&& func() \n";
    return global_val;
  }
};

int main() {
  A a;
  auto &t = a.func();
  std::cout << "global &:\t" << &global_val << '\n';
  std::cout << "&t:\t" << &t << '\n';
  return 0;
}
