#include "get_awaiter.hpp"
#include <coroutine>
#include <gtest/gtest.h>
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

// 测试成员函数 operator co_await
TEST(CoroutineAwaiterTest, MemberOperatorCoAwait) {
  member_awaiter m;
  auto awaiter = get_awaiter(m);
  static_assert(noexcept(get_awaiter(m)));
  testing::internal::CaptureStdout();
  awaiter.await_resume();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Resumed!\n");
}

// 测试全局 operator co_await
TEST(CoroutineAwaiterTest, GlobalOperatorCoAwait) {
  global_awaiter g;
  auto awaiter = get_awaiter(g);
  static_assert(noexcept(get_awaiter(g)));
  testing::internal::CaptureStdout();
  awaiter.await_resume();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Resumed!\n");
}

// 测试自身是 awaiter
TEST(CoroutineAwaiterTest, DirectAwaiter) {
  simple_awaiter s;
  auto awaiter = get_awaiter(s);
  testing::internal::CaptureStdout();
  awaiter.await_resume();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Resumed!\n");
}
