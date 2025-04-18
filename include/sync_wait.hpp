#pragma once
#include "is_awaiter.hpp"
namespace coro {

template <Awaitable T> auto sync_wait(T awaitable) {
  auto awaiter = get_awaiter(awaitable);
  while (!awaiter.await_ready()) {
    awaiter.await_suspend(std::noop_coroutine());
  }
  awaiter.await_resume();
}

} // namespace coro