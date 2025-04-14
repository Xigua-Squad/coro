#pragma once
#include <coroutine>
#include <iostream>

struct promise_base {
  struct final_awaitable {
    std::coroutine_handle<> continution_;
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
      /*std::cout << "final_awaiter :" << handle.address() << '\n';*/
      if (continution_) {
        continution_.resume();
      }
    }
    void await_resume() noexcept {}
  };
  auto initial_suspend() { return std::suspend_always{}; } // 惰性启动
  auto final_suspend() noexcept {
    return final_awaitable{continution_};
  } // 惰性结束
  // 如果final_suspend() 返回std::suspend_never  则协程会立即销毁
  void unhandled_exception() { std::terminate(); }
  std::coroutine_handle<> continution_;
};
// final_suspend的调用时机，c++协程机制会自动调用promise.final_suspend()
template <class T> class task;

template <class T> struct promise : public promise_base {
  void return_value(T value) { value_ = value; }
  task<T> get_return_object() noexcept;
  T result() { return value_; }
  T value_;
};

template <> struct promise<void> : public promise_base {
  void return_void() {}
  task<void> get_return_object() noexcept;
  void result() {}
};

template <class T> class task {
public:
  using promise_type = promise<T>;
  task() : coroutine_(nullptr) {}
  task(std::coroutine_handle<promise_type> coroutine) : coroutine_(coroutine) {
 
  }
  ~task() {
    if (coroutine_) {
      coroutine_.destroy();
    }
  }
  void resume() { coroutine_.resume(); }
  auto operator co_await() { return awaiter{coroutine_}; }

private:
  struct awaiter {
    std::coroutine_handle<promise_type> coro_;
    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
      coro_.promise().continution_ = handle; // 保存父协程
      // 立即执行本协程
      return coro_;
    }
    T await_resume() {
      return coro_.promise().result();
    } // result能够处理void和非void
  };

  std::coroutine_handle<promise_type> coroutine_;
};

template <class T> task<T> promise<T>::get_return_object() noexcept {
  return task<T>(std::coroutine_handle<promise>::from_promise(*this));
} // 模板只是定义，编译器不会实例化，所以代码提示不行

// 对于偏特化的模板类型，在其他地方定义的时候，不需要加template<>
task<void> promise<void>::get_return_object() noexcept {
  return task<void>(std::coroutine_handle<promise>::from_promise(*this)); //
}
// 从一个具有promise_type的对象中获取协程帧