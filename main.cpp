#include <iostream>
class p {
public:
  // p() = default;
  virtual ~p() {
    std::cout << "p 析构" << std::endl;
  } //  多态的时候不会调用子类的析构函数
};
class c : public p {
public:
  // c() = default;
  ~c() {
    std::cout << "c 析构" << std::endl;
  }
};
int main() {
  p *c = new class c;
  delete c;
  return 0;
}
