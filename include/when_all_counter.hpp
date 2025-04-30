#pragma once
#include <atomic>
#include <coroutine>
#include <cstddef>
namespace xigua::coro {
class when_all_counter {
public:
  when_all_counter(std::size_t count) noexcept
      : count_(count), awaitingCoroutine_(nullptr) {}
  bool is_ready() const noexcept {
    return static_cast<bool>(awaitingCoroutine_);
  }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept {
    awaitingCoroutine_ = awaitingCoroutine;
    return count_.fetch_sub(1, std::memory_order_acq_rel) > 1;
    // fetch_sub返回旧值
  }
  void notify_awaitable_completed() noexcept {
    if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      awaitingCoroutine_.resume();
    }
  }

protected:
  std::atomic<std::size_t> count_;
  std::coroutine_handle<> awaitingCoroutine_;
};
} // namespace xigua::coro