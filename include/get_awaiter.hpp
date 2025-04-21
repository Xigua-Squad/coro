#pragma once
#include "is_awaiter.hpp"
#include <utility>
// 检查是否有成员函数 operator co_await
template <typename T>
concept HasMemberCoAwait = requires(T &&value) {
  { static_cast<T &&>(value).operator co_await() };
};

// 检查是否有全局函数 operator co_await
template <typename T>
concept HasFreeCoAwait = requires(T &&value) {
  { operator co_await(static_cast<T &&>(value)) };
};

// 具有成员函数 operator co_await
template <typename T>
  requires HasMemberCoAwait<T>
decltype(auto) get_awaiter_impl(T &&value) noexcept(
    noexcept(static_cast<T &&>(value).operator co_await())) {
  return static_cast<T &&>(value).operator co_await();
}

// 具有全局函数 operator co_await
template <typename T>
  requires(HasFreeCoAwait<T> && !HasMemberCoAwait<T>)
decltype(auto) get_awaiter_impl(T &&value) noexcept(
    noexcept(operator co_await(static_cast<T &&>(value)))) {
  return operator co_await(static_cast<T &&>(value));
}

// 本身就是Awaiter
template <typename T>
  requires(Awaiter<T> && !HasMemberCoAwait<T> && !HasFreeCoAwait<T>)
decltype(auto) get_awaiter_impl(T &&value) {
  return static_cast<T &&>(value);
}

// get_awaiter 实现，拿到awaitable对象的awaiter
template <typename T>
decltype(auto) get_awaiter(T &&value) noexcept(
    noexcept(get_awaiter_impl(std::forward<T>(value)))) {
  return get_awaiter_impl(std::forward<T>(value));
}