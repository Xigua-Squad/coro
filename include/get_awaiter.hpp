#pragma once
#include "is_awaiter.hpp"
#include <utility>
// 检查是否有成员函数 operator co_await
template <typename T>
concept has_member_co_await = requires(T &&value) {
  { static_cast<T &&>(value).operator co_await() };
};

// 检查是否有全局函数 operator co_await
template <typename T>
concept has_global_co_await = requires(T &&value) {
  { operator co_await(static_cast<T &&>(value)) };
};

// get_awaiter_impl
template <typename T>
  requires has_member_co_await<T>
decltype(auto) get_awaiter_impl(T &&value) noexcept(
    noexcept(static_cast<T &&>(value).operator co_await())) {
  return static_cast<T &&>(value).operator co_await();
}

template <typename T>
  requires(has_global_co_await<T> && !has_member_co_await<T>)
decltype(auto) get_awaiter_impl(T &&value) noexcept(
    noexcept(operator co_await(static_cast<T &&>(value)))) {
  return operator co_await(static_cast<T &&>(value));
}

template <typename T>
  requires(is_awaiter<T> && !has_member_co_await<T> && !has_global_co_await<T>)
decltype(auto) get_awaiter_impl(T &&value) {
  return static_cast<T &&>(value);
}

// get_awaiter 实现
template <typename T>
decltype(auto) get_awaiter(T &&value) noexcept(
    noexcept(get_awaiter_impl(std::forward<T>(value)))) {
  return get_awaiter_impl(std::forward<T>(value));
}