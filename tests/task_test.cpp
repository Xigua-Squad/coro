#include "sync_wait.hpp"
#include "task.hpp"
#include <gtest/gtest.h>
#include <memory>
using namespace xigua;
TEST(TaskTest, Test1) {
  auto task = []() -> coro::task<int> { co_return 1; }();
  EXPECT_EQ(coro::sync_wait(task), 1);
}
TEST(TaskTest, Test2) {
  auto task = []() -> coro::task<int> { co_return 1; };
  auto task2 = [=]() -> coro::task<> {
    auto ret = co_await task();
    EXPECT_EQ(ret, 1);
  };
}
// 测试引用类型
TEST(TaskTest, Test3) {
  int val = 90;
  auto task = [&]() -> coro::task<int &> { co_return val; };
  auto task2 = [&]() -> coro::task<> {
    auto &ret = co_await task();
    ret = 100;
  };
  coro::sync_wait(task2());
  EXPECT_EQ(val, 100);
}
// 测试const引用类型
TEST(TaskTest, Test4) {
  const int val = 90;
  const int *val_addr = std::addressof(val);
  auto task = [&]() -> coro::task<const int &> { co_return val; };
  auto task2 = [&]() -> coro::task<> {
    auto &ret = co_await task();
    EXPECT_EQ(val_addr, std::addressof(val));
  };
  coro::sync_wait(task2());
}
