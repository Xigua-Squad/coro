#pragma once
#include "concepts/awaitable.hpp"
#include "manual_reset_event.hpp"
#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace xigua::coro {

class sync_wait_task_promise_base {
public:
  sync_wait_task_promise_base() noexcept = default;
  auto initial_suspend() noexcept { return std::suspend_always{}; }

protected:
  manual_reset_event *event_;

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

  sync_wait_task<R> get_return_object() noexcept;

  auto start(manual_reset_event &event) {
    event_ = &event;
    coroutine_type::from_promise(*this).resume();
  }
  template <typename U>
    requires std::is_constructible_v<R, U &&>
  void return_value(U &&value) {
    if constexpr (std::is_reference_v<R>) {
      value_.template emplace<value_type>(std::addressof(value));
    } else {
      value_.template emplace<value_type>(std::forward<U>(value));
    }
  }
  auto unhandled_exception() noexcept {
    value_.template emplace<std::exception_ptr>(std::current_exception());
  }
  auto final_suspend() noexcept {
    struct completion_notifier {
      auto await_ready() const noexcept { return false; }
      auto await_suspend(coroutine_type coroutine) const noexcept { return coroutine.promise().event_->set(); }
      void await_resume() noexcept {}
    };
    return completion_notifier{};
  }

  // (cv)int & result()
  // (cv)int && result()
  R &&result() {
    if (std::holds_alternative<std::exception_ptr>(value_)) {
      std::rethrow_exception(std::get<std::exception_ptr>(value_));
    }
    if constexpr (std::is_reference_v<R>) {
      return *std::get<value_type>(value_); // const T&
    } else {
      return std::move(std::get<value_type>(value_)); // T
    }
  }
  // 在cppcoro 中 result 的返回值是 T&& ，引用折叠
  // 1. int -> int&&
  // 2. const int -> const int&&
  // 3. int& -> int&
  // 4. const int& -> const int&

private:
  std::variant<std::monostate, value_type, std::exception_ptr> value_;
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
  auto unhandled_exception() noexcept { exception_ = std::current_exception(); }
  auto final_suspend() noexcept {
    struct completion_notifier {
      auto await_ready() const noexcept { return false; }
      auto await_suspend(coroutine_type coroutine) const noexcept { return coroutine.promise().event_->set(); }
      void await_resume() noexcept {}
    };
    return completion_notifier{};
  }

private:
  std::exception_ptr exception_;
};

template <class T>
class sync_wait_task {
public:
  using promise_type = sync_wait_task_promise<T>;
  // 新增构造函数
  explicit sync_wait_task(std::coroutine_handle<promise_type> handle)
      : handle_(handle) {}
  void start(manual_reset_event &event) {
    handle_.promise().start(event);
  }

  decltype(auto) result() { return handle_.promise().result(); }
  // 返回值会出现的情况
  // 1. (cv)T&&
  // 2. (cv)T&
  // 3. void

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
[[nodiscard]] auto sync_wait(A &&awaitable) -> R {
  manual_reset_event event;
  auto task = make_sync_wait_task(std::forward<A>(awaitable));
  task.start(event); // 开始执行task任务
  event.wait();
  if constexpr (std::is_void_v<R>) {
    task.result();
    return;
  } else {
    return task.result(); // 如果result返回T&&
  }
}
} // namespace xigua::coro