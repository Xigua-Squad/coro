#include "sync_wait.hpp"
#include "task.hpp"
#include <iostream>
#include <memory>
int a = 999;
struct awaitobj {
  auto await_ready() const noexcept {
    return false;
  }
  void await_suspend(std::coroutine_handle<> handle) const noexcept {
    std::cout << "await_suspend" << std::endl;
    handle.resume();
  }
  int await_resume() const noexcept {
    std::cout << "await_resume \n";
    return a;
  }
};
std::string str = "西瓜";
xigua::coro::task<std::string &> echo() {
  co_return str;
}
xigua::coro::task<> foo() {
  std::cout << "foo\n";
  co_return;
}
const std::string &fun() {
  return str;
}
int main() {
  xigua::coro::sync_wait(foo());

  return 0;
}
