#pragma once
#include "is_awaiter.hpp"

// 检测是否可以转换成 Awaiter（即能 `co_await`）
template <typename T>
concept Awaitable = requires(T t) {
  requires Awaiter<T> || Awaiter<decltype(t.operator co_await())> ||
               Awaiter<decltype(operator co_await(t))>;
};