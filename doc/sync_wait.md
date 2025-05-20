# sync_wait
### 关键
sync_wait 的关键之处在于如何在普通函数中执行协程任务
```cpp
int main() {
  auto tt = ff();
  std::cout << "地址：" << &tt << '\n';
  tt.start();
  return 0;
}
```
`可以直接通过这种coroutine.resume()的方式实现`

首先对于awaitbale类型的执行，需要使用co_await运算符去执行
* 对于void返回值类型，无需收集结果
* 对于void返回值类型，需要通过co_yeild收集结果，我觉得co_return也可以实现

### 必须显示调用co_return;去返回task<void>


### demo
```cpp
#include "concepts/awaitable.hpp"
#include "task.hpp"
#include <utility>
using namespace xigua::coro;
template <concepts::awaitable T>
  requires(std::is_void_v<concepts::awaitable_traits_t<T>>)
task<> make_task(T &&awaitbale) {
  co_await std::forward<T>(awaitbale);
}

template <concepts::awaitable T>
  requires(!std::is_void_v<concepts::awaitable_traits_t<T>>)
task<concepts::awaitable_traits_t<T>> make_task(T &&awaitbale) {
  // static_assert(std::is_same_v<concepts::awaitable_traits_t<T>, std::string
  // &>);
  auto &&res = co_await std::forward<T>(awaitbale);
  co_return res;
}

template <concepts::awaitable T> auto sync_wait(T &&awaitbale) {
  // static_assert(!std::is_same_v<concepts::awaitable_traits_t<T>, void>);
  auto t = ::make_task(std::forward<T>(awaitbale));
  t.start();
  if constexpr (!std::is_void_v<concepts::awaitable_traits_t<T>>) {
    return t.coroutine_.promise().result();
  }
}
task<void> func() {
  std::cout << "月\n";
  co_return;
}

task<std::string> fun() { co_return "硒鼓啊"; }

int main() {

  sync_wait([]() -> task<> {
    std::cout << "嘻嘻嘻";
    co_return;
  }());

  return 0;
}
```

## sync_wait需要返回的值类型
sync_wait 返回的值的类型，不是会完全继承awaitable的返回值类型
也就是await_resume的返回值类型
**返回值类型**
* `T` 普通类型
* `void` 类型
* `T&` 引用类型
* `const T&` 常量引用类型
* `T&&`
**以下类型是无意义的**
* `const T` 
## 测试demo
```cpp
template <class T> struct pp {

  template <class U> void f(U &&u) {
    T t = std::forward<U>(u);
    // 这里成功被推导成 int&
    t++;
  }
};

int main() {
  int a = 1;
  pp<int &> p;
  p.f(a); // 调用拷贝构造函数
  std::cout << a << '\n';
  return 0;
}
```


## sync_wait_event 同步原语的作用
```cpp
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <thread>

// 简单同步事件
struct event {
  std::mutex mtx;
  std::condition_variable cv;
  bool ready = false;

  void set() {
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    cv.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return ready; });
  }
};

struct task {
  struct promise_type {
    event *ev = nullptr;

    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept {
      struct notifier {
        bool await_ready() const noexcept { return false; }
        void
        await_suspend(std::coroutine_handle<promise_type> h) const noexcept {
          if (h.promise().ev) {
            h.promise().ev->set();
          }
        }
        void await_resume() noexcept {}
      };
      return notifier{};
    }
    void unhandled_exception() {}
    void return_void() {}
    task get_return_object() {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
  };

  task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
  void start(event &ev) {
    handle_.promise().ev = &ev;
    handle_.resume();
  }
  ~task() {
    if (handle_)
      handle_.destroy();
  }
  std::coroutine_handle<promise_type> handle_;
};

struct sleep_awaiter {
  std::chrono::milliseconds duration;
  bool await_ready() const noexcept { return duration.count() <= 0; }
  void await_suspend(std::coroutine_handle<> handle) const noexcept {
    std::thread([handle, duration = duration] {
      std::this_thread::sleep_for(duration);
      handle.resume();
    }).detach();
    // 这里另外开了一个线程负责协程任务的执行和恢复，所以不会阻塞主线程
    // 会导致主线程的执行流没有结果的返回
  }
  void await_resume() const noexcept {}
};

// 一个模拟异步操作的协程
task async_work() {
  std::cout << "[coroutine] step 1\n";
  co_await sleep_awaiter{std::chrono::milliseconds(1000)};
  std::cout << "[coroutine] step 2\n";
  co_return;
}

int main() {
  event ev;
  auto t = async_work();
  t.start(ev);
  // 程序流执行到这里的时候，t没有执行完成
  std::cout << "[main] waiting for coroutine...\n";
  ev.wait(); // 主线程会阻塞在这里，直到协程结束
  std::cout << "[main] coroutine finished!\n";
  return 0;
}
```