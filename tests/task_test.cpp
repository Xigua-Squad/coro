#include "task.hpp"
#include <gtest/gtest.h>
#include <memory>
coro::task<int> func1() { co_return 89; }
coro::task<std::string &> func2(std::string &str) { co_return str; }

coro::task<void> func3() {
  auto t1 = func1();
  co_await t1;
}
coro::task<void> main_task() {
  std::string str = "Hello";
  auto &ret = co_await func2(str);
  std::cout << "&str :" << &str << '\n';
  std::cout << "&ret :" << &ret << '\n';
}

TEST(CoroTest, TaskTest) {
  auto t = main_task();
  t.start();
}