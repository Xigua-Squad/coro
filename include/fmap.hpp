#pragma once
#include "awaitable_traits.hpp"
#include "concepts/awaitable.hpp"
#include "get_awaiter.hpp"
#include <coroutine>
#include <functional>
#include <type_traits>
#include <utility>
namespace xigua::coro {

template <typename Func, Awaitable A> class fmap_awaiter {
  using awaiter_t = typename awaitable_traits<A>::awaiter_t;
  Func &&func_;
  awaiter_t awaiter_; // 这里是不是也得是右值类型

public:
  fmap_awaiter(Func &&func, A &&awaitable) noexcept(
      std::is_nothrow_move_constructible_v<awaiter_t> &&
      noexcept(get_awaiter(std::forward<A>(awaitable))))
      : func_(std::forward<Func>(func)),
        awaiter_(get_awaiter(std::forward<A>(awaitable))) {}

  decltype(auto) await_ready() noexcept(
      noexcept(std::forward<awaiter_t>(awaiter_).await_ready())) {
    return std::forward<awaiter_t>(awaiter_).await_ready();
  }

  template <typename Promise>
  decltype(auto)
  await_suspend(std::coroutine_handle<Promise> coro) noexcept(noexcept(
      std::forward<awaiter_t>(awaiter_).await_suspend(std::move(coro)))) {
    return std::forward<awaiter_t>(awaiter_).await_suspend(std::move(coro));
  }

  template <
      typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume())>
  decltype(auto) await_resume() noexcept(
      noexcept(std::invoke(std::forward<Func>(func_),
                           std::forward<awaiter_t>(awaiter_).await_resume()))) {
    return std::invoke(std::forward<Func>(func_),
                       static_cast<awaiter_t &&>(awaiter_).await_resume());
  }

  template <
      typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume())>
  decltype(auto)
  await_resume() noexcept(noexcept(std::invoke(std::forward<Func>(func_))))
    requires std::is_void_v<AwaitResult>
  {
    std::forward<awaiter_t>(awaiter_).await_resume();
    std::invoke(std::forward<Func>(func_));
  }
};

template <typename Func, Awaitable A> class fmap_awaitable {
public:
  template <typename FuncArg, typename AwaitableArg>
    requires std::is_constructible_v<Func, FuncArg &&> and
                 std::is_constructible_v<A, AwaitableArg &&>
  explicit fmap_awaitable(FuncArg &&func, AwaitableArg &&awaitable) noexcept(
      std::is_nothrow_constructible_v<Func, FuncArg &&> &&
      std::is_nothrow_constructible_v<A, AwaitableArg &&>)
      : func_(std::forward<FuncArg>(func)),
        awaitable_(std::forward<AwaitableArg>(awaitable)) {}

  auto operator co_await() const & {
    return fmap_awaiter<const Func &, const A &>(func_, awaitable_);
  }
  auto operator co_await() && {
    return fmap_awaiter<Func &&, A &&>(std::move(func_), std::move(awaitable_));
  }
  auto operator co_await() & {
    return fmap_awaiter<Func &, A &>(func_, awaitable_);
  }

private:
  Func func_;
  A awaitable_;
};

template <typename Func> struct fmap_transform {
  explicit fmap_transform(Func &&func) noexcept(
      std::is_nothrow_move_constructible_v<Func>)
      : func_(func) {}
  Func func_;
};

template <typename Func, Awaitable A> auto fmap(Func &&func, A &&awaitable) {
  using FuncType = std::remove_cv_t<std::remove_reference_t<Func>>;
  using AwaitableType = std::remove_cv_t<std::remove_reference_t<A>>;
  return fmap_awaitable<FuncType, AwaitableType>(std::forward<Func>(func),
                                                 std::forward<A>(awaitable));
}

template <typename Func> auto fmap(Func &&func) {
  return fmap_transform<Func>(std::forward<Func>(func));
}
// 通过ADL实现查找|运算实现
template <typename T, typename Func>
decltype(auto) operator|(T &&value, fmap_transform<Func> &&transform) {
  return fmap(std::forward<Func>(transform.func_), std::forward<T>(value));
}
} // namespace xigua::coro