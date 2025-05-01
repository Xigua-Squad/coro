#include "fmap.hpp"
#include "sync_wait.hpp"
#include "task.hpp"
#include <cmath>
#include <iostream>
coro::task<std::string> get() { co_return "代拍"; }
coro::task<void> test() {
  auto t2 = get() | coro::fmap([](const std::string &str) {
              std::cout << "lambda 执行\n";
              std::cout << str << std::endl;
            });
  auto t3 = coro::fmap([](std::string) {}, get());
  co_await t2;
}

int main() {
  coro::sync_wait(test());
  return 0;
}
