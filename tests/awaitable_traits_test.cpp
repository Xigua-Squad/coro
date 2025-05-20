#include "concepts/awaitable.hpp"
#include <coroutine>
#include <gtest/gtest.h>
#include <type_traits>

using namespace xigua::coro::concepts;

// 1. 直接 Awaiter
struct DirectAwaiter {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const noexcept { return 123; }
};

// 2. Free operator co_await
struct FreeAwaitable {};
struct FreeAwaiter {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  double await_resume() const noexcept { return 3.14; }
};
FreeAwaiter operator co_await(const FreeAwaitable &) { return {}; }

// 3. Member operator co_await
struct MemberAwaitable {
  struct MemberAwaiter {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    const char *await_resume() const noexcept { return "hello"; }
  };
  MemberAwaiter operator co_await() const { return {}; }
};

// 4. 同时有 free 和 member co_await，优先 free
struct BothAwaitable {
  struct MemberAwaiter {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    float await_resume() const noexcept { return 2.71f; }
  };
  MemberAwaiter operator co_await() const { return {}; }
};
struct BothAwaiter {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  long await_resume() const noexcept { return 42L; }
};
BothAwaiter operator co_await(const BothAwaitable &) { return {}; }

// 5. 不可 await 的类型
struct NotAwaitable {};

TEST(AwaitableTraitsTest, DirectAwaiter) {
  static_assert(is_awaiter<DirectAwaiter>);
  static_assert(awaitable<DirectAwaiter>);
  static_assert(std::is_same_v<awaitable_traits_t<DirectAwaiter>, int>);
}

TEST(AwaitableTraitsTest, FreeCoAwait) {
  static_assert(has_free_co_await<FreeAwaitable>);
  static_assert(!has_member_co_await<FreeAwaitable>);
  static_assert(awaitable<FreeAwaitable>);
  static_assert(std::is_same_v<awaitable_traits_t<FreeAwaitable>, double>);
}

TEST(AwaitableTraitsTest, MemberCoAwait) {
  static_assert(!has_free_co_await<MemberAwaitable>);
  static_assert(has_member_co_await<MemberAwaitable>);
  static_assert(awaitable<MemberAwaitable>);
  static_assert(
      std::is_same_v<awaitable_traits_t<MemberAwaitable>, const char *>);
}

TEST(AwaitableTraitsTest, BothCoAwaitPrefersFree) {
  static_assert(has_free_co_await<BothAwaitable>);
  static_assert(has_member_co_await<BothAwaitable>);
  static_assert(awaitable<BothAwaitable>);
}

TEST(AwaitableTraitsTest, NotAwaitable) {
  static_assert(!awaitable<NotAwaitable>);
  // 不可 await 的类型不能实例化 awaitable_traits_t，否则编译报错
}
