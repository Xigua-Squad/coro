#pragma once
#include "concepts/awaitable.hpp"
#include <coroutine>
#include <exception>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <utility>
namespace xigua::coro {

template <typename T> class task;

class task_promise_base {
  // 实现final_awaiter
  struct final_awaiter {
    bool await_ready() const noexcept { return false; }
    template <typename Promise>
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
      // 到这里来的handle就是本协程
      auto continution = handle.promise().continution_;
      if (continution) {
        return continution;
      }
      // 测试
      return std::noop_coroutine();
    }
    void await_resume() const noexcept {}
  };

public:
  // 实现promise的部分函数
  auto initial_suspend() { return std::suspend_always{}; }
  auto final_suspend() noexcept { return final_awaiter{}; }
  void set_continution(std::coroutine_handle<> handle) noexcept {
    continution_ = handle;
  }

private:
  std::coroutine_handle<> continution_;
};

template <typename T> class task_promise : public task_promise_base {
public:
  task_promise() noexcept {}
  task<T> get_return_object() noexcept;
  void unhandled_exception() noexcept {
    new (static_cast<void *>(std::addressof(exception_)))
        std::exception_ptr(std::current_exception());
    result_type_ = result_type::exception;
  }

  template <typename Value>
    requires std::convertible_to<Value &&, T>
  void return_value(Value &&value) noexcept(
      std::is_nothrow_constructible_v<T, Value &&>) {
    ::new (static_cast<void *>(std::addressof(value_)))
        T(std::forward<Value>(value)); // 通过Value去构造T
    result_type_ = result_type::value;
  }

  decltype(auto) result() & {
    switch (result_type_) {
    case result_type::empty:
      throw std::runtime_error("task not started");
    case result_type::value:
      return value_;
    case result_type::exception:
      std::rethrow_exception(exception_);
    }
    throw std::logic_error("Unhandled result type");
  }

  decltype(auto) result() && {
    switch (result_type_) {
    case result_type::empty:
      throw std::runtime_error("task not started");
    case result_type::value:
      return std::move(value_);
    case result_type::exception:
      std::rethrow_exception(exception_);
    }
    throw std::logic_error("Unhandled result type");
  }

  ~task_promise() {
    switch (result_type_) {
    case result_type::value:
      value_.~T();
      break;
    case result_type::exception:
      exception_.~exception_ptr();
      break;
    default:
      break;
    }
  }

private:
  enum class result_type { empty, value, exception };
  result_type result_type_ = result_type::empty;
  union {
    T value_;
    std::exception_ptr exception_;
  };
};

template <> class task_promise<void> : public task_promise_base {
public:
  task_promise() noexcept {}
  task<void> get_return_object() noexcept;
  void return_void() noexcept {}
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }
  void result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

private:
  std::exception_ptr exception_;
};

// 引用版本，使co_return 可以返回引用
template <typename T> class task_promise<T &> : public task_promise_base {
public:
  task<T &> get_return_object() noexcept;
  void return_value(T &value) noexcept { value_ = std::addressof(value); }
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }
  T &result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return *value_;
  }

private:
  T *value_ = nullptr;
  std::exception_ptr exception_;
};

template <typename T = void> class task {
public:
  using promise_type = task_promise<T>;

private:
  struct awaitable_base {
    awaitable_base(std::coroutine_handle<promise_type> coroutine)
        : coroutine_(coroutine) {}
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> handle) const noexcept {
      coroutine_.promise().set_continution(handle);
      return coroutine_;
    }

    std::coroutine_handle<promise_type> coroutine_;
  };

public:
  task() : coroutine_(nullptr) {}
  task(task &&other) : coroutine_(other.coroutine_) {
    other.coroutine_ = nullptr;
  }
  explicit task(std::coroutine_handle<promise_type> coroutine)
      : coroutine_(coroutine) {}
  task(const task &) = delete;
  task &operator=(const task &) = delete;
  task &operator=(task &&other) noexcept {
    if (std::addressof(other) != this) {
      if (coroutine_) {
        coroutine_.destroy();
      } // 销毁当前协程
      coroutine_ = other.coroutine_;
      other.coroutine_ = nullptr;
    }
  }
  ~task() {
    if (coroutine_) {
      coroutine_.destroy();
    }
  }
  bool is_ready() const noexcept { return coroutine_ && coroutine_.done(); }
  void start() const noexcept {
    if (coroutine_) {
      coroutine_.resume();
    }
  }
  auto operator co_await() const & noexcept {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;
      decltype(auto) await_resume() {
        return this->coroutine_.promise().result();
      } // Use coroutine_ from task
    };
    return awaitable{coroutine_};
  }

  auto operator co_await() && noexcept {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;
      decltype(auto) await_resume() {
        return std::move(this->coroutine_)
            .promise()
            .result(); // Use coroutine_ from task
      }
    };
    return awaitable{coroutine_};
  }

  auto when_ready() {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;
      void await_resume() {}
    };
    return awaitable{coroutine_};
  }
  std::coroutine_handle<promise_type> coroutine_;

private:
};

template <typename T> task<T> task_promise<T>::get_return_object() noexcept {
  return task<T>{std::coroutine_handle<task_promise>::from_promise(*this)};
}

inline task<void> task_promise<void>::get_return_object() noexcept {
  return task<void>{
      std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}
template <typename T>
task<T &> task_promise<T &>::get_return_object() noexcept {
  return task<T &>{
      std::coroutine_handle<task_promise<T &>>::from_promise(*this)};
}

template <concepts::awaitable T>
auto make_task(T awaitable) -> task<std::remove_cvref_t<
    typename concepts::awaitable_traits<T>::await_result_t>> {
  co_return co_await std::forward<T>(awaitable);
}
} // namespace xigua::coro