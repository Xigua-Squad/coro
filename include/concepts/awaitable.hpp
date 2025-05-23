#pragma once
#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>

namespace xigua::coro::concepts {
template <typename T>
concept is_valid_await_suspend_result =
    std::is_void_v<T> || std::is_same_v<T, bool> ||
    std::is_same_v<T, std::coroutine_handle<>>;

template <typename T>
concept is_awaiter = requires(T t, std::coroutine_handle<> h) {
  { t.await_ready() } -> std::convertible_to<bool>;
  { t.await_suspend(h) } -> is_valid_await_suspend_result;
  { t.await_resume() };
};
template <typename T>
concept has_free_co_await = requires {
  { operator co_await(std::declval<T>()) } -> is_awaiter;
};
template <typename T>
concept has_member_co_await = requires {
  { std::declval<T>().operator co_await() } -> is_awaiter;
};

template <typename T>
concept awaitable = requires {
  requires is_awaiter<T> || has_free_co_await<T> || has_member_co_await<T>;
};

template <typename T> struct awaitable_traits;
template <is_awaiter T> struct awaitable_traits<T> {
  using awaiter_t = T;
  using await_result_t = decltype(std::declval<T &&>().await_resume());
};
template <typename T>
  requires(has_free_co_await<T> && !has_member_co_await<T>)
struct awaitable_traits<T> {
  using awaiter_t = decltype(operator co_await(std::declval<T>()));
  using await_result_t = decltype(std::declval<awaiter_t &&>().await_resume());
};
template <typename T>
  requires(has_member_co_await<T> && !has_free_co_await<T>)
struct awaitable_traits<T> {
  using awaiter_t = decltype(std::declval<T &&>().operator co_await());
  using await_result_t = decltype(std::declval<awaiter_t &&>().await_resume());
};
template <typename T>
  requires(has_free_co_await<T> && has_member_co_await<T>)
struct awaitable_traits<T> {
  static_assert(!sizeof(T),
                "Type T should not have both free and member co_await!");
};

template <typename T>
using awaitable_traits_t = typename awaitable_traits<T>::await_result_t;

} // namespace xigua::coro::concepts