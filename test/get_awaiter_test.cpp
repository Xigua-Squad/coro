#include "get_awaiter.hpp"
#include <coroutine>
#include <iostream>
#include <type_traits>

// 一个简单的 awaiter 类型
struct simple_awaiter {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept { std::cout << "Resumed!\n"; }
};

// 一个定义了成员函数 operator co_await 的类型
struct member_awaiter {
  simple_awaiter operator co_await() const noexcept { return {}; }
};

// 一个定义了全局 operator co_await 的类型
struct global_awaiter {};
simple_awaiter operator co_await(const global_awaiter &) noexcept { return {}; }

int main() {
  // 测试成员函数 operator co_await
  member_awaiter m;
  auto awaiter1 = get_awaiter(m);
  static_assert(noexcept(get_awaiter(m)));
  awaiter1.await_resume();

  // 测试全局 operator co_await
  global_awaiter g;
  auto awaiter2 = get_awaiter(g);
  static_assert(noexcept(get_awaiter(g)));
  awaiter2.await_resume();

  // 测试自身是 awaiter
  simple_awaiter s;
  auto awaiter3 = get_awaiter(s);
  awaiter3.await_resume();

  return 0;
}