#pragma once
#include "broken_promise.hpp"
#include <atomic>
#include <coroutine>
#include <exception>
#include <new>
#include <type_traits> // std::aligned_storage
#include <utility>
template <class T> class shared_task;

struct shared_task_waiter {
  std::coroutine_handle<> continution_;
  shared_task_waiter *next_ = nullptr;
};
class shared_task_promise_base {
  friend struct final_awaiter;
  struct final_awaiter { // 在本协程结束的时候调
    std::coroutine_handle<> continution_;
    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
      // 这里的handle就是本协程
      shared_task_promise_base &promise = handle.promise();
      constexpr void *readyFlag = &promise;
      void *waiters =
          promise.waiters_.exchange(readyFlag, std::memory_order_acq_rel);
      if (waiters != nullptr) { // 有等待者
        shared_task_waiter *waiter = static_cast<shared_task_waiter *>(waiters);
        while (waiter->next_ != nullptr) {
          auto *next = waiter->next_;
          waiter->continution_.resume(); // 恢复等待的协程
          waiter = next;
        }
        // 恢复最后一个等待协程
        waiter->continution_.resume();
      }
    }

    void await_resume() noexcept {};
  };

public:
  shared_task_promise_base() noexcept
      : refCount_(1), waiters_(&this->waiters_), exception_(nullptr) {}
  std::suspend_always initial_suspend() noexcept { return {}; }; // 惰性执行
  final_awaiter final_suspend() noexcept { return final_awaiter{}; }
  void unhandled_exception() noexcept { exception_ = std::current_exception(); }

  // 协程是否执行完成，即数据是否准备就绪
  bool is_ready() const noexcept {
    const void *const readyFlag = this;
    return waiters_.load(std::memory_order_acquire) == readyFlag;
  }

  void add_ref() noexcept { refCount_.fetch_add(1, std::memory_order_relaxed); }

  bool try_detach() noexcept {
    return refCount_.fetch_sub(1, std::memory_order_acq_rel) != 1;
  }

  bool try_await(shared_task_waiter *waiter,
                 std::coroutine_handle<> coroutine) {
    // 这个coroutine就是该shared_task协程的句柄，而非co_await的调用者
    const void *readyFlag = this;             // 表示协程执行完成
    const void *notStarted = &this->waiters_; // 表示协程尚未启动
    void *startedNoWaiter = nullptr;
    void *oldWaiters = waiters_.load(std::memory_order_acquire);
    if (oldWaiters == notStarted &&
        waiters_.compare_exchange_strong(oldWaiters, startedNoWaiter,
                                         std::memory_order_relaxed)) {
      // 变成启动状态，但是没有等待者
      coroutine.resume();
      oldWaiters = waiters_.load(std::memory_order_acquire);
    }
    do {
      if (oldWaiters == readyFlag) {
        return false; // 无需挂起，直接返回
      }
      // 协程任务尚未执行完成，添加协程到等待队列
      waiter->next_ = static_cast<shared_task_waiter *>(oldWaiters);
    } while (!waiters_.compare_exchange_weak(
        oldWaiters, static_cast<void *>(waiter), std::memory_order_release,
        std::memory_order_acquire));
    return true;
  }

protected:
  bool completed_with_unhandled_exception() const noexcept {
    return exception_ != nullptr;
  }

  void rethrow_if_unhandled_exception() {
    if (exception_ != nullptr) {
      std::rethrow_exception(exception_);
    }
  }

private:
  std::atomic<uint32_t> refCount_; // 引用计数
  // waiter_的含义
  // - nullptr 表示协程已经开始，但是没有等待者
  // - &promise/this 表示协程已经结束，并且结果已经准备好了
  // - &this->waiter_ 表示协程尚未启动
  // - 其他值，表示指向等待列表(shared_task_waiter*类型)
  std::atomic<void *> waiters_;
  std::exception_ptr exception_;
};

template <typename T> // 非void
class shared_task_promise : public shared_task_promise_base {
public:
  shared_task_promise() noexcept = default;

  ~shared_task_promise() {
    if (this->is_ready() && !this->completed_with_unhandled_exception()) {
      std::launder(reinterpret_cast<T *>(&valueStorage))
          ->~T(); // 显示调用析构函数
    }
  }

  // return_value 用于构造并返回一个值
  template <typename Value>
    requires std::is_convertible_v<Value &&,
                                   T> // 检查是否可以通过Value&&隐式转换成T
  void return_value(Value &&value) noexcept(
      std::is_nothrow_constructible_v<T, Value &&>) {
    new (&valueStorage)
        T(std::forward<Value>(value)); // 在 valueStorage 上构造 T
  }

  T &result() {
    this->rethrow_if_unhandled_exception(); // 让抛出异常这一步延迟到result被使用的时候
    return *std::launder(reinterpret_cast<T *>(&valueStorage));
  }

private:
  std::aligned_storage_t<sizeof(T), alignof(T)> valueStorage;
  // alignas(T) char valueStorage[sizeof(T)];
  //  典型的原地构造(placement new) + 类型擦除 + 手动生命周期管理
  //  alignas(T)确保这块内存的起始地址对齐方式满足类型T的对齐要求
  //  作用
  //  1.推迟T的构造时机
};

template <> // void 版本特化
class shared_task_promise<void> : public shared_task_promise_base {
public:
  shared_task_promise() noexcept = default;

  shared_task<void> get_return_object() noexcept;

  void return_void() noexcept {}

  void result() {
    this->rethrow_if_unhandled_exception(); // 重抛异常
  }
};

// 引用特化版本
template <typename T>
class shared_task_promise<T &> : public shared_task_promise_base {
public:
  shared_task_promise() noexcept = default;

  shared_task<T &> get_return_obejct() noexcept;

  void return_value(T &value) noexcept { value_ = std::addressof(value); }

  T &result() {
    this->rethrow_if_unhandled_exception();
    return *value_;
  }

private:
  T *value_;
};

// 分给shared_task的功能，好像就只剩下一个awaitable了
template <class T> class [[nodiscard]] shared_task {
public:
  using promise_type = shared_task_promise<T>;
  using value_type = T;

private:
  struct awaitable_base {
    std::coroutine_handle<promise_type> coroutine_;
    shared_task_waiter waiter_;
    awaitable_base(std::coroutine_handle<promise_type> coroutine) noexcept
        : coroutine_(coroutine) {}

    bool await_ready() const noexcept {
      return !coroutine_ || coroutine_.promise().is_ready();
    }
    bool await_suspend(std::coroutine_handle<> awaiter) noexcept {
      waiter_.continution_ = awaiter; // 保存父协程句柄
      return coroutine_.promise().try_await(&waiter_, coroutine_);
    }
  };

public:
  shared_task() noexcept : coroutine_(nullptr) {}

  explicit shared_task(std::coroutine_handle<promise_type> coroutine)
      : coroutine_(coroutine) {}
  shared_task(shared_task &&other) noexcept : coroutine_(other.coroutine_) {
    other.coroutine_ = nullptr;
  }
  shared_task(const shared_task &other) noexcept
      : coroutine_(other.coroutine_) {
    // 需要检查句柄的有效性，对于被std::move()的shared_task句柄是无效的，还有被无参构造的shared_task也是无效的
    if (coroutine_) {
      coroutine_.promise().add_ref();
    }
  }
  ~shared_task() { destroy(); }
  shared_task &operator=(shared_task &&other) noexcept {
    if (&other != this) {
      destroy();
      coroutine_ = other.coroutine_;
      other.coroutine_ = nullptr;
    }
    return *this;
  }
  shared_task &operator=(const shared_task &other) noexcept {
    // 如果两个指向的是同一个协程，那么就属于是重复的赋值了，直接退出
    // 可以放置 shtask = shtask 这种情况
    if (coroutine_ != other.coroutine_) {
      destroy();
      coroutine_ = other.coroutine_;
      if (coroutine_) {
        coroutine_.promise().add_ref();
      }
    }
    return *this;
  }
  void swap(shared_task &other) noexcept {
    std::swap(coroutine_, other.coroutine_);
  }
  bool is_ready() const noexcept {
    return !coroutine_ || coroutine_.promise().is_ready();
  }
  auto operator co_await() const noexcept {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;
      decltype(auto) await_resume() {
        if (!this->coroutine_) {
          throw broken_promise();
        }
        return this->coroutine_.promise().result();
      }
    };
    return awaitable{coroutine_};
  }

private:
  template <typename U>
  friend bool operator==(const shared_task<U> &,
                         const shared_task<U> &) noexcept;
  void destroy() {
    if (coroutine_) {
      if (!coroutine_.promise().try_detach()) {
        coroutine_.destroy();
      }
    }
  }
  std::coroutine_handle<promise_type> coroutine_;
};

template <typename U>
inline bool operator==(const shared_task<U> &l,
                       const shared_task<U> &r) noexcept {
  return l.coroutine_ == r.coroutine_;
}
template <typename T> void swap(shared_task<T> &a, shared_task<T> &b) {
  a.swap(b);
}