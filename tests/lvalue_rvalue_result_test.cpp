#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <type_traits>
#include <utility>
class test {
public:
  test() { std::cout << "test" << std::endl; }
  ~test() { std::cout << "~test" << std::endl; }
  test(const test &) { std::cout << "test copy constructor" << std::endl; }
  test(test &&) { std::cout << "test move constructor" << std::endl; }
  test &operator=(const test &) {
    std::cout << "test copy assignment" << std::endl;
    return *this;
  }
  test &operator=(test &&) {
    std::cout << "test move assignment" << std::endl;
    return *this;
  }
  int value = 89;
};

template <typename T> class promise {
public:
  //   void setValue(T &&t) { // 这里是右值引用，而不是万能引用
  //     ::new (&value_) T(std::forward<T>(t));
  //     // 这里实现了，能够准确调用test的拷贝构造还是移动构造
  //   }
  template <typename U> void setValue(U &&t) { // 这里是万能引用
    std::cout << "setValue 调用\n";
    ::new (&value_) T(std::forward<U>(t));
  }
  T &result() & {
    std::cout << "result & 调用\n";
    return value_;
  }
  T &&result() && {
    std::cout << "result && 调用\n";
    return std::move(value_);
  } // 返回 T&&
  union {
    T value_;
  };
  promise() {} // Explicitly define a default constructor
  ~promise() { value_.~T(); }
};

template <class T> class promise<T &> {
  // 引用特化版本
public:
  void setValue(T &t) {
    std::cout << "setValue 引用特化版本调用\n";
    value_ = std::ref(t);
  }
  T &result() {
    std::cout << "result 引用特化版本调用\n";
    return value_.get();
  }
  std::reference_wrapper<T> value_;
};

template <class T> class task {
public:
  decltype(auto) get() & {
    std::cout << "get & 调用\n";
    return promise_.result();
  }
  decltype(auto) get() && {
    std::cout << "get && 调用\n";
    return std::move(promise_).result();
  }
  // 这里会调用&&版本的result函数
  promise<T> promise_;
  // T 会保留const & 类型
};
template <typename T> struct test_reference {
  // 引用类型是可以传递过来的
  // 这里的T是const test &
  test_reference() {}
};
// 函数的返回值是右值类型有啥意义吗
// 测试test类型
TEST(LValueAndRValue, Test) {
  task<test> a;
  test aa;
  a.promise_.setValue(aa);
  decltype(auto) ret = a.get(); // 右值版本
  ret.value = 100;
  std::cout << aa.value << '\n';
  static_assert(
      std::is_rvalue_reference_v<decltype(std::move(a).get())>); // 右值版本
}