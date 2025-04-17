#pragma once
#include "get_awaiter.hpp"

template <typename T>
  requires requires(T &&value) {
    { get_awaiter(std::forward<T>(value)) };
  }
struct awaitable_traits {
  using awaiter_t = decltype(get_awaiter(std::declval<T &&>()));
  using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
};