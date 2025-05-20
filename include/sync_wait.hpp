#pragma once
#include "concepts/awaitable.hpp"
#include <utility>
template <class T> class sync_wait_task;

namespace xigua::coro {
template <concepts::awaitable A, typename R = concepts::awaitable_traits_t<A>>
static auto make_sync_wait_task(A &&awaitable) -> sync_wait_task<R> {
  if constexpr (std::is_void_v<R>) {
    co_await awaitable;
    co_return;
  } else {
    co_return co_await std::forward<A>(awaitable);
  }
}
} // namespace xigua::coro