#include "sync_wait.hpp"
#include "task.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
using namespace xigua::coro;
namespace {
class object {
public:
  object() {}
  ~object() {}
  object(const object &) { std::cout << "object 复制构造" << std::endl; }
  object &operator=(const object &) {
    std::cout << "object 赋值构造" << std::endl;
    return *this;
  }
  object(object &&) { std::cout << "object 移动构造" << std::endl; }
  object &operator=(object &&) {
    std::cout << "object 移动赋值构造" << std::endl;
    return *this;
  }
};
} // namespace
TEST(SyncWaitTest, ReturnValue) {
  // 测试返回值为void的情况
  std::string str;
  auto t1 = [&]() -> task<void> {
    str = "12345";
    co_return;
  };
  sync_wait(t1());
  EXPECT_EQ(str, "12345");

  // 测试返回值为普通类型的情况
  auto t2 = [&]() -> task<int> {
    co_return 123;
  };
  int result = sync_wait(t2());
  EXPECT_EQ(result, 123);

  // 测试返回值为引用&
  auto t3 = [&]() -> task<std::string &> {
    co_return str;
  };
  std::string &result2 = sync_wait(t3());
  EXPECT_EQ(std::addressof(result2), std::addressof(str));

  object obj1;
  // 测试返回值为右值（移动语义）
  auto t4 = [&]() -> task<object> {
    co_return std::move(obj1);
  };
  auto &&obj2 = sync_wait(t4());
  static_assert(std::is_same_v<object &&, decltype(obj2)>, "result2 should be std::string");

  std::string ss;
  // 测试返回值为左值（拷贝语义）
  auto t5 = [&]() -> task<std::string> {
    co_return ss;
  };
  auto &&obj3 = sync_wait(t5());
  static_assert(std::is_same_v<std::string &&, decltype(obj3)>, "result3 should be std::string");

  // 测试返回值是否保留cv限定符
  std::string obj4;
  auto t6 = [&]() -> task<const std::string &> {
    co_return obj4;
  };
  auto &&result3 = sync_wait(t6());
  static_assert(std::is_same_v<const std::string &, decltype(result3)>, "result3 should be const std::string&");
}

// 测试异常
TEST(SyncWaitTest, ExceptionHandling) {
  auto t1 = [&]() -> task<void> {
    throw std::runtime_error("test exception");
    co_return;
  };
  try {
    sync_wait(t1());
    FAIL() << "Expected exception was not thrown";
  } catch (const std::runtime_error &e) {
    EXPECT_STREQ(e.what(), "test exception");
  }
  // 测试void返回值类型，异常是否被正确捕获
  auto t2 = [&]() -> task<void> {
    throw std::runtime_error("test exception");
  };
  EXPECT_THROW(sync_wait(t2()), std::runtime_error);
  // 测试非void返回值类型，异常是否被正确捕获
  auto t3 = [&]() -> task<int> {
    throw std::runtime_error("test exception");
    co_return 123;
  };
  EXPECT_THROW(auto t = sync_wait(t3()), std::runtime_error);
}
// EXPECT_THROW(sync_wait(t1()), std::runtime_error);