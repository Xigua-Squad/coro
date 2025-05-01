#include "fmap.hpp"
#include "sync_wait.hpp"
#include "task.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

using namespace xigua::coro;

// 基础功能测试
TEST(FmapTest, ValueTransform) {
  auto get_num = []() -> task<int> { co_return 42; };
  auto double_num = [](int x) { return x * 2; };

  sync_wait([&]() -> task<> {
    auto result = co_await fmap(double_num, get_num());
    EXPECT_EQ(result, 84);
  }());
}

TEST(FmapTest, VoidAwaitable) {
  bool executed = false;
  std::cout << "test value addr:" << std::addressof(executed) << '\n';
  auto do_nothing = []() -> task<> { co_return; };
  auto set_flag = [&]() {
    std::cout << "set_flag value addr:" << std::addressof(executed) << '\n';
    executed = true;
  };

  sync_wait([=, &executed]() -> task<> {
    co_await fmap(set_flag, do_nothing());
    std::cout << "sync_wait value addr:" << std::addressof(executed) << '\n';
    EXPECT_TRUE(executed);
  }());
  /*
    test value addr:0x7ffdef5579ef
    set_flag value addr:0x7ffdef5579ef
    sync_wait value addr:0x7ffdef557a09
  */
}

// 链式调用测试
TEST(FmapTest, PipelineStyle) {
  auto get_str = []() -> task<std::string> { co_return "hello"; };
  auto add_world = [](std::string s) { return s + " world"; };
  auto to_upper = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
  };

  sync_wait([&]() -> task<> {
    auto result = co_await (get_str() | fmap(add_world) | fmap(to_upper));
    EXPECT_EQ(result, "HELLO WORLD");
  }());
}

// 异常处理测试
TEST(FmapTest, ExceptionPropagation) {
  auto throw_exception = []() -> task<int> {
    throw std::runtime_error("test error");
    co_return 0;
  };
  auto safe_transform = [](int) { return 42; };

  EXPECT_THROW(
      { sync_wait(fmap(safe_transform, throw_exception())); },
      std::runtime_error);
}

// 移动语义测试
// TEST(FmapTest, MoveOnlyTypes) {
//   struct PP {
//     PP() = default;
//     PP(PP &&) = default; // 明确移动构造
//   };
// }
// // 引用传递测试
// TEST(FmapTest, ReferencePreservation) {
//   int external_value = 50;
//   auto get_ref = [&]() -> task<int &> { co_return external_value; };
//   auto modify = [](int &x) -> int & {
//     x *= 2;
//     return x;
//   };

//   sync_wait([&]() -> task<> {
//     auto &result = co_await fmap(modify, get_ref());
//     EXPECT_EQ(result, 100);
//     EXPECT_EQ(external_value, 100); // 验证原值被修改
//   }());
// }

// // 多参数lambda测试
// TEST(FmapTest, MultiParamLambda) {
//   struct Point {
//     int x, y;
//   };
//   auto get_point = []() -> task<Point> { co_return Point{3, 4}; };
//   auto calc_length = [](Point p) { return std::sqrt(p.x * p.x + p.y * p.y);
//   };

//   sync_wait([]() -> task<> {
//     auto len = co_await fmap(calc_length, get_point());
//     EXPECT_DOUBLE_EQ(len, 5.0);
//   }());
// }

// // 覆盖所有operator co_await重载
TEST(FmapTest, AllCoAwaitVariants) {
  // const& 版本
  auto get_const = []() -> task<const int> { co_return 10; };
  sync_wait(fmap([](const int x) { return x + 1; }, get_const()));

  // & 版本
  int val = 20;
  auto get_ref = [&]() -> task<int &> { co_return val; };
  sync_wait(fmap([](int &x) { x += 5; }, get_ref()));

  // && 版本
  auto get_rvalue = []() -> task<std::vector<int>> {
    co_return std::vector{1, 2, 3};
  };
  // sync_wait(fmap([](std::vector<int> &&v) { return v.size(); },
  // get_rvalue()));
}

// // 空awaitable测试
// TEST(FmapTest, EmptyAwaitable) {
//   struct EmptyAwaitable {
//     bool await_ready() const noexcept { return true; }
//     void await_suspend(auto) const noexcept {}
//     void await_resume() const noexcept {}
//   };

//   bool called = false;
//   EmptyAwaitable empty;
//   sync_wait(fmap([&] { called = true; }, empty));
//   ASSERT_TRUE(called);
// }

// // 嵌套fmap测试
// TEST(FmapTest, NestedFmap) {
//   auto get_num = []() -> task<int> { co_return 2; };
//   auto square = [](int x) { return x * x; };
//   auto to_string = [](int x) { return std::to_string(x); };

//   sync_wait([]() -> task<> {
//     auto str = co_await fmap(to_string, fmap(square, get_num()));
//     EXPECT_EQ(str, "4");
//   }());
// }
