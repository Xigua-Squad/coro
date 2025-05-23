#pragma once

#include <atomic>
#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

// #ifdef _WIN32
#include <condition_variable>
#include <mutex>
#include <thread>
// #endif

namespace xigua::coro {
// #ifdef __linux__
// class manual_reset_event {
// public:
//   void wait() {
//     int old_value = value_.load(std::memory_order_acquire);
//     while (old_value == 0) {
//       int ret = syscall(SYS_futex, &value_, FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
//                         old_value, nullptr, nullptr, 0);
//       // FUTEX_PRIVATE  表示futex是进程私有的（多线程共享），避免跨进程同步开销
//       // 当前线程会检查value_的值是否等于old_value，如果等于则阻塞挂起。
//       // 等待被FUTEX_WAKE唤醒
//       if (ret == -1) {
//         if (errno == EAGAIN)
//           return;
//       }
//       old_value = value_.load(std::memory_order_acquire);
//     } // while防止以外唤醒
//   }
//   void set() {
//     value_.store(1, std::memory_order_release);
//     syscall(SYS_futex, &value_, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX,
//             nullptr, nullptr, 0);
//     // MAX_INT 最大唤醒线程数
//   }
//   void reset() { value_.store(0, std::memory_order_relaxed); }

// private:
//   std::atomic<int> value_{0};
// };

// #endif

// #ifdef _WIN32

class manual_reset_event {
public:
  void wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return value_.load(std::memory_order_acquire); });
  }
  void set() {
    value_.store(1, std::memory_order_release);
    cv_.notify_all();
  }
  void reset() { value_.store(0, std::memory_order_relaxed); }

private:
  std::atomic<int> value_{0};
  std::condition_variable cv_;
  std::mutex mtx_;
};
// #endif

} // namespace xigua::coro
