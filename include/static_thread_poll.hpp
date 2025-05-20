#pragma once
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <queue>
#include <thread>

class static_thread_poll {
public:
  static_thread_poll() {
    worker_ = std::thread([this] { run(); });
  }

  ~static_thread_poll() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_.store(true);
    }
    cv_.notify_all();
    if (worker_.joinable())
      worker_.join();
  }

  struct schedule_operation {
    static_thread_poll *poll_;
    explicit schedule_operation(static_thread_poll *poll) : poll_(poll) {}
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
      {
        std::lock_guard<std::mutex> lock(poll_->mutex_);
        poll_->queue_.push(handle);
      }
      poll_->cv_.notify_one();
    }
    void await_resume() noexcept {}
  };

  schedule_operation schedule() { return schedule_operation{this}; }

private:
  void run() {
    while (true) {
      std::coroutine_handle<> handle;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || stop_.load(); });

        if (stop_.load())
          break;

        handle = queue_.front();
        queue_.pop();
      }
      if (handle)
        handle.resume();
    }
  }

  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::coroutine_handle<>> queue_;
  std::atomic<bool> stop_{false};
  std::thread worker_;
};