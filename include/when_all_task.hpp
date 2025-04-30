#pragma once
#include "when_all_counter.hpp"
#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
namespace xigua::coro {
class when_all_counter;
template <typename Task_Container> class when_all_ready_awaitable;
template <typename Result> class when_all_task;
template <typename Result> class when_all_task_promise final {
public:
  using coroutine_handle_t =
      std::coroutine_handle<when_all_task_promise<Result>>;
  when_all_task_promise() noexcept = default;
  auto get_return_object() noexcept {
    return coroutine_handle_t::from_promise(*this);
  }
  std::suspend_always initial_suspend() noexcept { return {}; }
  auto final_suspend() noexcept {
    struct completion_notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coroutine_handle_t coro) const noexcept {
        coro.promise().counter_->notify_awaitable_completed();
      }
      void await_resume() const noexcept {}
    };
    return completion_notifier{};
  }
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }

  void return_void() noexcept { assert(false); }
  auto yield_value(Result &&result) noexcept {
    result = std::addressof(result);
    return final_suspend();
  }
  void start(when_all_counter &counter) noexcept {
    counter_ = &counter;
    coroutine_handle_t::from_promise(*this).resume();
  }
  Result &result() & {
    rethrow_if_exception();
    return *result_;
  }
  Result &&result() && {
    rethrow_if_exception();
    return std::forward<Result>(*result_);
  }

private:
  void rethrow_if_exception() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }
  when_all_counter *counter_;
  std::exception_ptr exception_;
  std::add_pointer_t<Result> result_;
};

template <> class when_all_task_promise<void> final {
public:
  using coroutine_handle_t = std::coroutine_handle<when_all_task_promise<void>>;
  when_all_task_promise() noexcept = default;
  auto get_return_object() noexcept {
    return coroutine_handle_t::from_promise(*this);
  }
  std::suspend_always initial_suspend() noexcept { return {}; }
  auto final_suspend() noexcept {
    struct completion_notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coroutine_handle_t coro) const noexcept {
        coro.promise().counter_->notify_awaitable_completed();
      }
      void await_resume() const noexcept {}
    };
    return completion_notifier{};
  }
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }
  void return_void() noexcept {}
  void result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

private:
  when_all_counter *counter_;
  std::exception_ptr exception_;
};

} // namespace xigua::coro