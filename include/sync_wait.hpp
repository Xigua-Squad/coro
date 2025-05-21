#pragma once
#include "concepts/awaitable.hpp"
#include "manual_reset_event.hpp"
#include <coroutine>
#include <exception>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

namespace xigua::coro {

class sync_wait_task_promise_base {
public:
  sync_wait_task_promise_base() noexcept = default;
  auto initial_suspend() noexcept { return std::suspend_always{}; }
  auto unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

protected:
  manual_reset_event *event_;
  std::exception_ptr exception_;
  ~sync_wait_task_promise_base() = default;
};

template <typename T>
class sync_wait_task;

template <typename R>
class sync_wait_task_promise : public sync_wait_task_promise_base {
public:
  using coroutine_type = std::coroutine_handle<sync_wait_task_promise>;
  using rm_const_reference_t = std::remove_const_t<std::remove_reference_t<R>>;
  using value_type = std::conditional_t<std::is_reference_v<R>, std::add_pointer_t<R>, rm_const_reference_t>;
  using stored_type = std::variant<std::monostate, value_type, std::exception_ptr>;

  sync_wait_task<R> get_return_object() noexcept;

  auto start(manual_reset_event &event) {
    event_ = &event;
    coroutine_type::from_promise(*this).resume();
  }
  template <typename U>
    requires std::is_constructible_v<R, U &&>
  void return_value(U &&value) {
    std::cout << "return_value:" << value << '\n';
    if constexpr (std::is_reference_v<R>) {
      value_.template emplace<value_type>(std::addressof(value));
    } else {
      value_.template emplace<value_type>(std::forward<U>(value));
    }
  }
  auto final_suspend() noexcept {
    struct completion_notifier {
      auto await_ready() const noexcept { return false; }
      auto await_suspend(coroutine_type coroutine) const noexcept { return coroutine.promise().event_->set(); }
      void await_resume() noexcept {}
    };
    return completion_notifier{};
  }

  decltype(auto) result() {
    if (std::holds_alternative<std::exception_ptr>(value_)) {
      std::rethrow_exception(std::get<std::exception_ptr>(value_));
    }
    if constexpr (std::is_reference_v<R>) {
      // static_assert(std::is_same_v<value_type, int *>);
      return *std::get<value_type>(value_); // const T&
    } else {
      // static_assert(std::is_same_v<value_type, int>);
      return std::get<value_type>(value_); // T
    }
  }

private:
  stored_type value_; // T& ,const T&, T
};
// addpoint可以处理引用吗
template <>
class sync_wait_task_promise<void> : public sync_wait_task_promise_base {
public:
  using coroutine_type = std::coroutine_handle<sync_wait_task_promise<void>>;
  void return_void() {}

  auto start(manual_reset_event &event) {
    event_ = &event;
    coroutine_type::from_promise(*this).resume();
  }
  sync_wait_task<void> get_return_object() noexcept;
  void result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }
  auto final_suspend() noexcept {
    struct completion_notifier {
      auto await_ready() const noexcept { return false; }
      auto await_suspend(coroutine_type coroutine) const noexcept { return coroutine.promise().event_->set(); }
      void await_resume() noexcept {}
    };
    return completion_notifier{};
  }

private:
};

template <class T>
class sync_wait_task {
public:
  using promise_type = sync_wait_task_promise<T>;
  // 新增构造函数
  explicit sync_wait_task(std::coroutine_handle<promise_type> handle)
      : handle_(handle) {}
  auto promise() & -> promise_type & { return handle_.promise(); }
  auto promise() const & -> const promise_type & { return handle_.promise(); }
  auto promise() && -> promise_type && { return std::move(handle_.promise()); }

  decltype(auto) result() { return handle_.promise().result(); }

private:
  std::coroutine_handle<promise_type> handle_;
};

template <typename T>
auto sync_wait_task_promise<T>::get_return_object() noexcept -> sync_wait_task<T> {
  return sync_wait_task<T>(coroutine_type::from_promise(*this));
}

inline auto sync_wait_task_promise<void>::get_return_object() noexcept -> sync_wait_task<void> {
  return sync_wait_task<void>(coroutine_type::from_promise(*this));
}

template <concepts::awaitable A, typename R = concepts::awaitable_traits_t<A>>
static auto make_sync_wait_task(A &&awaitable) -> sync_wait_task<R> {

  if constexpr (std::is_void_v<R>) {
    co_await awaitable;
    co_return;
  } else {
    co_return co_await std::forward<A>(awaitable);
  }
}

template <concepts::awaitable A, typename R = concepts::awaitable_traits_t<A>>
auto sync_wait(A &&awaitable) -> R {
  manual_reset_event event;
  auto task = make_sync_wait_task(std::forward<A>(awaitable));
  task.promise().start(event); // 开始执行task任务
  event.wait();
  if constexpr (std::is_void_v<R>) {
    task.result();
    return;
  } else {
    return task.result();
  }
}
} // namespace xigua::coro