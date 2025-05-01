#pragma once
#include "awaitable_traits.hpp"
#include "concepts/awaitable.hpp"
#include "lightweight_manual_reset_event.hpp"
#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <type_traits>
#include <unistd.h>
#include <utility>
namespace xigua::coro {

template <typename T> class sync_wait_task;
template <typename T> class sync_wait_task_promise final {
  using coroutine_handle_t = std::coroutine_handle<sync_wait_task_promise<T>>;

public:
  sync_wait_task_promise() noexcept = default;
  void start(lightweight_manual_reset_event &event) {
    event_ = &event;
    coroutine_handle_t::from_promise(*this).resume();
  }
  auto get_return_object() noexcept {
    return sync_wait_task<T>(coroutine_handle_t::from_promise(*this));
  }

  std::suspend_always initial_suspend() noexcept { return {}; }

  auto final_suspend() noexcept {
    struct completion_notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coroutine_handle_t coroutine) const noexcept {
        coroutine.promise().event_->set();
        // 唤醒因为event_阻塞的线程
      }
      void await_resume() const noexcept {}
    };
    return completion_notifier{};
  }

  auto yield_value(T &&value) noexcept {
    result_ = std::addressof(value);
    return final_suspend();
  }

  void return_void() noexcept {
    assert(false);
    // 这里应该不会被调用
  }

  void unhandled_exception() noexcept { exception_ = std::current_exception(); }

  T &&result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return static_cast<T &&>(*result_);
  }

private:
  lightweight_manual_reset_event *event_ = nullptr;
  std::remove_reference_t<T> *result_;
  std::exception_ptr exception_;
};

template <> class sync_wait_task_promise<void> {
  using coroutine_handle_t =
      std::coroutine_handle<sync_wait_task_promise<void>>;

public:
  sync_wait_task_promise() noexcept = default;
  void start(lightweight_manual_reset_event &event) {
    event_ = &event;
    coroutine_handle_t::from_promise(*this).resume();
  }
  std::suspend_always initial_suspend() noexcept { return {}; }
  auto get_return_object() noexcept {
    return coroutine_handle_t::from_promise(*this);
  } // 返回的是一个std::coroutine_handle协程句柄
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }
  auto final_suspend() noexcept {
    struct completion_notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coroutine_handle_t coroutine) const noexcept {
        coroutine.promise().event_->set();
        // 唤醒因为event_阻塞的线程
      }
      void await_resume() const noexcept {}
    };
    return completion_notifier{};
  }

  void return_void() noexcept {}

  void result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

private:
  lightweight_manual_reset_event *event_ = nullptr;
  std::exception_ptr exception_;
};

template <typename T> class sync_wait_task final {
public:
  using promise_type = sync_wait_task_promise<T>;
  using coroutine_handle_t = std::coroutine_handle<promise_type>;
  sync_wait_task(coroutine_handle_t coroutine) : coroutine_(coroutine) {}
  sync_wait_task(sync_wait_task &&other) noexcept
      : coroutine_(std::exchange(other.coroutine_, coroutine_handle_t{})) {}
  sync_wait_task(const sync_wait_task &) = delete;
  sync_wait_task &operator=(const sync_wait_task &) = delete;
  void start(lightweight_manual_reset_event &event) {
    coroutine_.promise().start(event);
  }
  decltype(auto) result() { return coroutine_.promise().result(); }

private:
  coroutine_handle_t coroutine_;
};

// 非 void 返回值的版本
template <Awaitable A>
  requires(!std::is_void_v<typename awaitable_traits<A &&>::await_result_t>)
sync_wait_task<typename awaitable_traits<A &&>::await_result_t>
make_sync_wait_task(A &&awaitable) {
  co_yield co_await std::forward<A>(awaitable);
}

// void 返回值的版本
template <Awaitable A>
  requires std::is_void_v<typename awaitable_traits<A &&>::await_result_t>
sync_wait_task<void> make_sync_wait_task(A &&awaitable) {
  co_await std::forward<A>(awaitable);
}

template <Awaitable A>
auto sync_wait(A &&awaitable) -> awaitable_traits<A &&>::await_result_t {
  auto task = make_sync_wait_task(std::forward<A>(awaitable));
  // 把awaitable转成sync_wait_task
  lightweight_manual_reset_event event;
  task.start(event);
  event.wait();
  return task.result();
}

} // namespace xigua::coro